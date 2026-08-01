#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define firmwareVersion "1.0.0"
/* =========================================================
   BOARD DETECTION & PIN CONFIG
   ========================================================= */
#define PIN_IR_RECEIVER   14//D5
#define PIN_IR_SENDER     05//D1
#define PIN_BUTTON        13//D7
#define PIN_STATUS_LED    04//D2
#define PIN_PWM_INPUT     12//D6
#define UART_RX_PIN       00//D3
#define UART_TX_PIN       02//D4
#define PIN_LM35_ENABLE   16//D0
#define PIN_LM35_SENSOR   A0


/* ================= SYSTEM CONFIG ================= */
#define EEPROM_TOTAL_SIZE     4096
#define IR_RAW_BUFFER_SIZE    200   // Reduced for EEPROM fit
#define IR_CARRIER_FREQ       38

#define BUTTON_LONG_PRESS_MS  3000UL
#define IR_LEARN_TIMEOUT_MS   20000UL

#define TOTAL_IR_KEYS         17   // ON, OFF, 16→30
#define LEARNING_SESSION_TIMEOUT_MS 150000UL   // 2 minutes
#define BRIGHTNESS_CHANGE_THRESHOLD 3


SoftwareSerial extUart(UART_RX_PIN, UART_TX_PIN);
/* ================= IR OBJECTS ================= */
IRrecv irReceiver(PIN_IR_RECEIVER, IR_RAW_BUFFER_SIZE, 15, true);
IRsend irSender(PIN_IR_SENDER);
decode_results irResults;

/* ================= IR STORAGE ================= */
uint16_t irRawData[TOTAL_IR_KEYS][IR_RAW_BUFFER_SIZE];
uint16_t irRawLength[TOTAL_IR_KEYS] = {0};

/* ================= SYSTEM STATE ================= */
bool isLearningMode = false;
bool isAcOn = false;
int currentTemperature = 24;
bool lm35TriggerProcessed = false;
bool lm35ModeEnabled = false;
int uartTargetTemp = 24;
int defaultAcTemp = 24;
int lastBrightness = -1;
int lastMappedTemp = -1;

/* ================= PWM ================= */
volatile unsigned long pwmRiseTime = 0;
volatile unsigned long pwmPeriod = 0;
volatile unsigned long pwmHighTime = 0;
bool eepromClearedThisSession = false;
/* =========================================================
   EEPROM HELPERS
   ========================================================= */
int getEEPROMBaseAddress(uint8_t keyIndex) {
  return keyIndex * 240; // enough for 200-length buffer
}

/* =========================================================
   CLEAR EEPROM
   ========================================================= */
void clearEEPROM()
{
    EEPROM.begin(EEPROM_TOTAL_SIZE);
    for (int i = 0; i < EEPROM_TOTAL_SIZE; i++)
    {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    EEPROM.end();

    memset(irRawLength, 0, sizeof(irRawLength));
    Serial.println("EEPROM Cleared");
}

void saveIRCodeToEEPROM(uint8_t keyIndex) {
  EEPROM.begin(EEPROM_TOTAL_SIZE);

  int base = getEEPROMBaseAddress(keyIndex);
  uint16_t len = irRawLength[keyIndex];

  EEPROM.write(base, len & 0xFF);
  EEPROM.write(base + 1, len >> 8);

  for (uint16_t i = 0; i < len; i++) {
    EEPROM.write(base + 2 + i * 2, irRawData[keyIndex][i] & 0xFF);
    EEPROM.write(base + 3 + i * 2, irRawData[keyIndex][i] >> 8);
  }

  EEPROM.commit();
  EEPROM.end();
}

bool loadIRCodeFromEEPROM(uint8_t keyIndex) {
  EEPROM.begin(EEPROM_TOTAL_SIZE);

  int base = getEEPROMBaseAddress(keyIndex);
  uint16_t len = EEPROM.read(base) | (EEPROM.read(base + 1) << 8);

  if (len == 0 || len > IR_RAW_BUFFER_SIZE) {
    EEPROM.end();
    return false;
  }

  irRawLength[keyIndex] = len;

  for (uint16_t i = 0; i < len; i++) {
    irRawData[keyIndex][i] =
      EEPROM.read(base + 2 + i * 2) |
      (EEPROM.read(base + 3 + i * 2) << 8);
  }

  EEPROM.end();
  return true;
}

/* =========================================================
   BRIGHTNESS
   ========================================================= */
int getBrightness(float duty)
{
    const int POINTS = 18;
    const float TOLERANCE = 0.5;

    const float dutyTable[POINTS] =
    {
        2.73, 11.33, 21.68, 31.37,
        39.45, 47.85, 55.47, 62.11,
        68.16, 73.60, 78.91, 83.20,
        86.72, 89.84, 92.95, 94.92,
        96.48, 97.85
    };

    const int brightTable[POINTS] =
    {
        100, 95, 90, 85,
        80, 75, 70, 65,
        60, 55, 50, 45,
        40, 35, 30, 25,
        20, 15
    };

    /* Upper & lower limits */
    if (duty <= (dutyTable[0] + TOLERANCE))
        return 100;

    if (duty >= (dutyTable[POINTS - 1] - TOLERANCE))
        return 15;

    /* Find nearest matching point */
    for (int i = 0; i < POINTS; i++)
    {
        if (fabs(duty - dutyTable[i]) <= TOLERANCE)
        {
            return brightTable[i];
        }
    }

    /* Fallback: nearest point */
    float minDiff = 999.0;
    int nearestIndex = 0;
    for (int i = 0; i < POINTS; i++)
    {
        float diff = fabs(duty - dutyTable[i]);

        if (diff < minDiff)
        {
            minDiff = diff;
            nearestIndex = i;
        }
    }
    return brightTable[nearestIndex];
}

int calculateBrightnessPercent() {

  unsigned long highTime = pulseIn(PIN_PWM_INPUT, HIGH);
  unsigned long lowTime  = pulseIn(PIN_PWM_INPUT, LOW);
  unsigned long period = highTime + lowTime;
  if (period < 0) 
      return 15;

  float frequency = 1000000.0 / period;
  float dutyCycle = (highTime * 100.0) / period;
  int currentBrightness = getBrightness(dutyCycle);
  delay(500);
  return currentBrightness;
}

/* =========================================================
   TEMP MAPPING
   ========================================================= */
int mapBrightnessToTemperature(int b) {

  if (b < 15) return -1;

  if (b <= 20) return 16;
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
   IR SEND
   ========================================================= */
void sendIRCommand(uint8_t keyIndex) {
  if (irRawLength[keyIndex] > 0) {
    irSender.sendRaw(irRawData[keyIndex], irRawLength[keyIndex], IR_CARRIER_FREQ);
  }
}

void sendACTemperature(int temp) {
  if (temp < 16) temp = 16;
  if (temp > 30) temp = 30;

  uint8_t index = temp - 14; // 16→2

  Serial.printf("Sending TEMP %d\n", temp);
  sendIRCommand(index);
}

/* =========================================================
   AC CONTROL
   ========================================================= */
void updateACState(int brightness) {

  static int lastTemp = -1;

  if (brightness < 15) {
    if (isAcOn) {
      Serial.println("AC OFF");
      sendIRCommand(1);
      isAcOn = false;
    }
    return;
  }

  int targetTemp = mapBrightnessToTemperature(brightness);
  Serial.printf("Brightness: %d → Temp: %d\n", brightness, targetTemp);
  if (!isAcOn) {
    Serial.println("AC ON");
    sendIRCommand(0);
    delay(2000);
    isAcOn = true;
  }

  if (targetTemp != lastTemp) {
//    Serial.printf("Brightness: %d → Temp: %d\n", brightness, targetTemp);
    sendACTemperature(targetTemp);
    currentTemperature = targetTemp;
    lastTemp = targetTemp;
  }
}

/* =========================================================
   LEARNING
   ========================================================= */
bool learnIRCommand(uint8_t keyIndex,unsigned long sessionStart) {

  unsigned long start = millis();
  while (millis() - start < IR_LEARN_TIMEOUT_MS) {

    if (millis() - sessionStart >= LEARNING_SESSION_TIMEOUT_MS) {
      Serial.println("Learning session timeout");
      return false;
    }

    digitalWrite(PIN_STATUS_LED, LOW);
    delay(120);
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(120);

    if (irReceiver.decode(&irResults)) {
      if (!eepromClearedThisSession)
      {
          clearEEPROM();
          eepromClearedThisSession = true;
      }

      uint16_t len = irResults.rawlen - 1;
      if (len > IR_RAW_BUFFER_SIZE) len = IR_RAW_BUFFER_SIZE;

      irRawLength[keyIndex] = len;

      for (uint16_t i = 1; i <= len; i++) {
        irRawData[keyIndex][i - 1] = irResults.rawbuf[i] * kRawTick;
      }

      saveIRCodeToEEPROM(keyIndex);
      irReceiver.resume();
      Serial.printf("Saved Key- %d\n", keyIndex);
      delay(500);
      return true;
      
    }
  }

  return false;
}


/* =========================================================
   READ LM35 SENSOR
   ========================================================= */
float readLM35Temperature()
{
    int adc = analogRead(PIN_LM35_SENSOR);

    float voltage = adc * (3.3 / 1023.0);

    float tempC = voltage * 100.0;

    return tempC;
}

/* =========================================================
   LM35 TRIGGER PROCESS
   ========================================================= */
void processLM35Mode()
{
    bool trigger = digitalRead(PIN_LM35_ENABLE);

    if (!trigger)
    {
        lm35TriggerProcessed = false;
        return;
    }

    if (lm35TriggerProcessed)
        return;

    float roomTemp = readLM35Temperature();
    Serial.printf("LM35 Temp = %.1f C\n", roomTemp);

    if (!isAcOn)
    {
        Serial.println("LM35 Trigger -> AC ON");
        sendIRCommand(0);
        delay(2000);
        isAcOn = true;
    }

    int targetTemp = uartTargetTemp;
    if (targetTemp < 16 || targetTemp > 30)
    {
        targetTemp = defaultAcTemp;
    }
    Serial.printf("LM35 Trigger -> Set Temp %d\n", targetTemp);
    sendACTemperature(targetTemp);
    currentTemperature = targetTemp;
    lm35TriggerProcessed = true;
}
/* =========================================================
   EXTERNAL UART PROCESS
   ========================================================= */
void processExternalUART()
{
    if(isLearningMode)
        return;
        
    while (extUart.available())
    {
        String cmd = extUart.readStringUntil('\n');
        cmd.trim();
        cmd.toUpperCase();

        /* ================= AC ON ================= */
        if (cmd == "ON")
        {
            sendIRCommand(0);
            isAcOn = true;

            extUart.println("ON_ACK");
            Serial.println("UART: AC ON");
        }

        /* ================= AC OFF ================= */
        else if (cmd == "OFF")
        {
            sendIRCommand(1);
            isAcOn = false;

            extUart.println("OFF_ACK");
            Serial.println("UART: AC OFF");
        }

        /* ================= TEMP ================= */
        else if (cmd.startsWith("TEMP:"))
        {
            int temp = cmd.substring(5).toInt();

            if (temp >= 16 && temp <= 30)
            {
                sendACTemperature(temp);
                uartTargetTemp = temp;
                currentTemperature = temp;
                isAcOn = true;

                extUart.printf("TEMP_%d_ACK\r\n", temp);

                Serial.printf("UART: TEMP %d\n", temp);
            }
            else
            {
                extUart.println("TEMP_INVALID");
            }
        }

        /* ================= STATUS ================= */
        else if (cmd == "STATUS")
        {
            extUart.printf(
                "STATUS,AC=%s,TEMP=%d\r\n",
                isAcOn ? "ON" : "OFF",
                currentTemperature
            );
        }

        /* ================= UNKNOWN ================= */
        else
        {
            extUart.println("CMD_UNKNOWN");
        }
    }
}


void startIRLearningSequence()
{
    Serial.println("Start Learning Mode");
    isLearningMode = true;
    eepromClearedThisSession = false;
    unsigned long sessionStart = millis();
    for (uint8_t i = 0; i < TOTAL_IR_KEYS; i++) {

        if (millis() - sessionStart >= LEARNING_SESSION_TIMEOUT_MS) {
            Serial.println("Learning mode expired");
            break;
        }

        if (i == 0)
            Serial.println("Learn AC ON");
        else if (i == 1)
            Serial.println("Learn AC OFF");
        else
            Serial.printf("Learn TEMP %d\n", 14 + i);

        if (!learnIRCommand(i, sessionStart)) {
            break;
        }

        delay(300);
    }

    Serial.println("Learning mode exited");
    isLearningMode = false;
}

/* =========================================================
   SETUP
   ========================================================= */
void setup() {

  Serial.begin(115200);
  extUart.begin(115200);
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_PWM_INPUT, INPUT);

  digitalWrite(PIN_STATUS_LED, HIGH);

  irReceiver.enableIRIn();
  irSender.begin();
  for (uint8_t i = 0; i < TOTAL_IR_KEYS; i++) {
    loadIRCodeFromEEPROM(i);
  }

  Serial.println("\n\n\n");
  Serial.println("System Ready");
}

/* =========================================================
   LOOP
   ========================================================= */
void loop() {

  static unsigned long btnStart = 0;
  static bool longPress = false;

  bool pressed = (digitalRead(PIN_BUTTON) == LOW);

  if (pressed && btnStart == 0) {
    btnStart = millis();
    longPress = false;
  }

  if (pressed && !longPress) {
    if (millis() - btnStart >= BUTTON_LONG_PRESS_MS) {
      longPress = true;
      startIRLearningSequence();
    }
  }

  if (!pressed) {
    btnStart = 0;
  }

  static unsigned long lastCheck = 0;

  if (millis() - lastCheck > 2000)
  {
      int brightness = calculateBrightnessPercent();
      if (brightness >= 0)
      {
          if (lastBrightness < 0 ||
              abs(brightness - lastBrightness) >= BRIGHTNESS_CHANGE_THRESHOLD)
          {
              updateACState(brightness);
              lastBrightness = brightness;
          }
      }
      lastCheck = millis();
  }
  
  processExternalUART();
  processLM35Mode();
  
}
