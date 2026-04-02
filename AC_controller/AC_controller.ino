#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <EEPROM.h>

/* =========================================================
   BOARD DETECTION & PIN CONFIG
   ========================================================= */

#if defined(ESP8266)

  #define PIN_IR_RECEIVER   D5
  #define PIN_IR_SENDER     D1
  #define PIN_BUTTON        D7
  #define PIN_STATUS_LED    D2
  #define PIN_PWM_INPUT     D6

#elif defined(ESP32)

  #define PIN_IR_RECEIVER   14
  #define PIN_IR_SENDER     5
  #define PIN_BUTTON        13
  #define PIN_STATUS_LED    2
  #define PIN_PWM_INPUT     12

#else
  #error "Unsupported board"
#endif

/* ================= SYSTEM CONFIG ================= */
#define EEPROM_TOTAL_SIZE     2048
#define IR_RAW_BUFFER_SIZE    350
#define IR_CARRIER_FREQ       38

#define BUTTON_LONG_PRESS_MS  5000UL
#define IR_LEARN_TIMEOUT_MS   20000UL

/* ================= IR OBJECTS ================= */
IRrecv irReceiver(PIN_IR_RECEIVER, IR_RAW_BUFFER_SIZE, 15, true);
IRsend irSender(PIN_IR_SENDER);
decode_results irResults;

/* ================= IR STORAGE ================= */
// 0: ON, 1: OFF, 2: TEMP+, 3: TEMP-
uint16_t irRawData[4][IR_RAW_BUFFER_SIZE];
uint16_t irRawLength[4] = {0};

/* ================= SYSTEM STATE ================= */
bool isLearningMode = false;
bool isAcOn = false;
int currentTemperature = 24;

/* ================= PWM MEASUREMENT ================= */
volatile unsigned long pwmRiseTime = 0;
volatile unsigned long pwmPeriod = 0;
volatile unsigned long pwmHighTime = 0;

/* =========================================================
   EEPROM HELPERS
   ========================================================= */

int getEEPROMBaseAddress(uint8_t keyIndex) {
  const int addressMap[4] = {0, 800, 1200, 1600};
  return addressMap[keyIndex];
}

void saveIRCodeToEEPROM(uint8_t keyIndex) {
  EEPROM.begin(EEPROM_TOTAL_SIZE);

  int baseAddr = getEEPROMBaseAddress(keyIndex);
  uint16_t length = irRawLength[keyIndex];

  EEPROM.write(baseAddr, length & 0xFF);
  EEPROM.write(baseAddr + 1, length >> 8);

  for (uint16_t i = 0; i < length; i++) {
    EEPROM.write(baseAddr + 2 + i * 2, irRawData[keyIndex][i] & 0xFF);
    EEPROM.write(baseAddr + 3 + i * 2, irRawData[keyIndex][i] >> 8);
  }

  EEPROM.commit();
  EEPROM.end();
}

bool loadIRCodeFromEEPROM(uint8_t keyIndex) {
  EEPROM.begin(EEPROM_TOTAL_SIZE);

  int baseAddr = getEEPROMBaseAddress(keyIndex);
  uint16_t length = EEPROM.read(baseAddr) | (EEPROM.read(baseAddr + 1) << 8);

  if (length == 0 || length > IR_RAW_BUFFER_SIZE) {
    EEPROM.end();
    return false;
  }

  irRawLength[keyIndex] = length;

  for (uint16_t i = 0; i < length; i++) {
    irRawData[keyIndex][i] =
      EEPROM.read(baseAddr + 2 + i * 2) |
      (EEPROM.read(baseAddr + 3 + i * 2) << 8);
  }

  EEPROM.end();
  return true;
}

/* =========================================================
   PWM INTERRUPT
   ========================================================= */

void IRAM_ATTR pwmInterruptHandler() {
  static bool lastState = LOW;
  bool currentState = digitalRead(PIN_PWM_INPUT);

  unsigned long now = micros();

  if (currentState && !lastState) {
    pwmPeriod = now - pwmRiseTime;
    pwmRiseTime = now;
  }

  if (!currentState && lastState) {
    pwmHighTime = now - pwmRiseTime;
  }

  lastState = currentState;
}

/* =========================================================
   BRIGHTNESS CALCULATION (Original Logic)
   ========================================================= */

int calculateBrightnessPercent() {
  if (pwmPeriod == 0) return -1;

  float duty = (pwmHighTime * 100.0) / pwmPeriod;

  int brightness = 100 - duty; // Inverted PWM

  if (brightness < 0) brightness = 0;
  if (brightness > 100) brightness = 100;

  return brightness;
}

/* =========================================================
   ORIGINAL TEMP MAPPING (UNCHANGED)
   ========================================================= */

int mapBrightnessToTemperature(int b) {

  if (b >= 15 && b <= 20) return 16;
  if (b <= 25) return 17;
  if (b <= 30) return 18;
  if (b <= 35) return 19;
  if (b <= 40) return 20;
  if (b <= 45) return 21;
  if (b <= 50) return 22;
  if (b <= 55) return 23;
  if (b <= 60) return 24;
  if (b <= 65) return 25;
  if (b <= 70) return 26;
  if (b <= 75) return 27;
  if (b <= 80) return 28;
  if (b <= 85) return 29;
  if (b <= 99) return 30;

  return 24;
}

/* =========================================================
   IR SEND HELPER
   ========================================================= */

void sendIRCommand(uint8_t keyIndex) {
  if (irRawLength[keyIndex] > 0) {
    irSender.sendRaw(irRawData[keyIndex], irRawLength[keyIndex], IR_CARRIER_FREQ);
  }
}

/* =========================================================
   AC CONTROL (UNCHANGED LOGIC)
   ========================================================= */

void updateACState(int brightness) {

  static int lastTargetTemp = -1;

  if (brightness == 0) {
    if (isAcOn) {
      Serial.println("AC OFF");
      sendIRCommand(1);
      isAcOn = false;
    }
    return;
  }

  int targetTemp = mapBrightnessToTemperature(brightness);

  if (!isAcOn) {
    Serial.println("AC ON → Set 24");
    sendIRCommand(0);
    delay(2000);
    currentTemperature = 24;
    isAcOn = true;
  }

  if (targetTemp == lastTargetTemp) return;

  Serial.printf("Brightness: %d → Temp: %d\n", brightness, targetTemp);

  while (currentTemperature < targetTemp) {
    sendIRCommand(2);
    currentTemperature++;
    delay(500);
  }

  while (currentTemperature > targetTemp) {
    sendIRCommand(3);
    currentTemperature--;
    delay(500);
  }

  lastTargetTemp = targetTemp;
}

/* =========================================================
   IR LEARNING
   ========================================================= */

bool learnIRCommand(uint8_t keyIndex) {

  Serial.printf("Learning Key-%d\n", keyIndex + 1);
  unsigned long startTime = millis();

  while (millis() - startTime < IR_LEARN_TIMEOUT_MS) {

    digitalWrite(PIN_STATUS_LED, LOW);
    delay(120);
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(120);

    if (irReceiver.decode(&irResults)) {

      uint16_t rawLength = irResults.rawlen - 1;
      if (rawLength > IR_RAW_BUFFER_SIZE) rawLength = IR_RAW_BUFFER_SIZE;

      irRawLength[keyIndex] = rawLength;

      for (uint16_t i = 1; i <= rawLength; i++) {
        irRawData[keyIndex][i - 1] = irResults.rawbuf[i] * kRawTick;
      }

      saveIRCodeToEEPROM(keyIndex);
      irReceiver.resume();
      return true;
    }
  }

  return false;
}

void startIRLearningSequence() {

  isLearningMode = true;

  for (uint8_t i = 0; i < 4; i++) {
    learnIRCommand(i);
    delay(300);
  }

  isLearningMode = false;
}

/* =========================================================
   SETUP
   ========================================================= */

void setup() {

  Serial.begin(115200);

#if defined(ESP8266)
  Serial.println("Running on ESP8266");
#elif defined(ESP32)
  Serial.println("Running on ESP32");
#endif

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_PWM_INPUT, INPUT);

  digitalWrite(PIN_STATUS_LED, HIGH);

  irReceiver.enableIRIn();
  irSender.begin();

  attachInterrupt(digitalPinToInterrupt(PIN_PWM_INPUT), pwmInterruptHandler, CHANGE);

  for (uint8_t i = 0; i < 4; i++) {
    loadIRCodeFromEEPROM(i);
  }

  Serial.println("System Ready");
}

/* =========================================================
   LOOP
   ========================================================= */

void loop() {

  static unsigned long buttonPressStart = 0;
  static bool longPressHandled = false;

  bool isButtonPressed = (digitalRead(PIN_BUTTON) == LOW);

  if (isButtonPressed && buttonPressStart == 0) {
    buttonPressStart = millis();
    longPressHandled = false;
  }

  if (isButtonPressed && !longPressHandled) {
    if (millis() - buttonPressStart >= BUTTON_LONG_PRESS_MS) {
      longPressHandled = true;
      startIRLearningSequence();
    }
  }

  if (!isButtonPressed) {
    buttonPressStart = 0;
  }

  static unsigned long lastUpdateTime = 0;

  if (millis() - lastUpdateTime > 2000) {

    int brightness = calculateBrightnessPercent();

    if (brightness >= 0) {
      updateACState(brightness);
    }

    lastUpdateTime = millis();
  }
}