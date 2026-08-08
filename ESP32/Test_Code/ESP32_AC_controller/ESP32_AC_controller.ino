#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <EEPROM.h>
#include <SimpleDHT.h>

#define firmwareVersion "0.0.1"
// Testing in ESP32 + ESP8266 
// Demo //Learning disabled
// UART disable and DHT using not LM35
/* =========================================================
   BOARD DETECTION & PIN CONFIG
   ========================================================= */
/* =========================================================
   BOARD DETECTION & PIN CONFIG
   ========================================================= */

#if defined(ESP8266)/* ================= ESP8266 ================= */
#include <SoftwareSerial.h>

  #define PIN_IR_RECEIVER   14   // D5
  #define PIN_IR_SENDER     5    // D1
  #define PIN_BUTTON        13   // D7
  #define PIN_STATUS_LED    4    // D2
  #define PIN_PWM_INPUT     12   // D6
  #define UART_RX_PIN       0    // D3
  #define UART_TX_PIN       2    // D4
  #define PIN_LM35_ENABLE   16   // D0
  #define PIN_DHT_SENSOR    15   // D8

#elif defined(ESP32)/* ================= ESP32 ================= */

  #define PIN_IR_RECEIVER   14
  #define PIN_IR_SENDER     5
  #define PIN_BUTTON        13
  #define PIN_STATUS_LED    2
  #define PIN_PWM_INPUT     12
  #define UART_RX_PIN       16
  #define UART_TX_PIN       17
  #define PIN_LM35_ENABLE   4
  #define PIN_DHT_SENSOR    15
#else
  #error "Unsupported board"
#endif


SimpleDHT11 dht11(PIN_DHT_SENSOR);
/* ================= SYSTEM CONFIG ================= */
// #define EEPROM_TOTAL_SIZE     4096
#define IR_RAW_BUFFER_SIZE    750   // Reduced for EEPROM fit
#define IR_CARRIER_FREQ       38
#define AC_OFF_LOCATION       0
#define AC_ON_LOCATION        1
#define MIN_IR_FRAME_LENGTH   100

#define TOTAL_IR_KEYS         18   // ON, OFF, 16→30
#define BRIGHTNESS_CHANGE_THRESHOLD 5

#if defined(ESP8266)
  SoftwareSerial extUart(UART_RX_PIN, UART_TX_PIN);
#elif defined(ESP32)
  HardwareSerial extUart(2);
#endif
/* ================= IR OBJECTS ================= */
IRrecv irReceiver(PIN_IR_RECEIVER, IR_RAW_BUFFER_SIZE, 15, true);
IRsend irSender(PIN_IR_SENDER);
decode_results irResults;

/* ================= IR STORAGE ================= */
uint8_t irRawLength = 453;
uint16_t rawData[453] = {30112, 49836,  3466, 1634,  468, 1224,  468, 378,  468, 378,  468, 376,  468, 376,  468, 378,  466, 356,  490, 376,  468,
     352,  494, 376,  468, 378,  468, 354,  490, 1200,  492, 354,  490, 356,  490, 376,  468, 376,  468, 378,  468, 354,  492, 376,  468, 1222,  468, 1224,  468, 354,  492, 376,  470, 376,  468, 376,  468, 
376,  468, 356,  490, 352,  492, 378,  468, 1222,  468, 356,  488, 1224,  468, 1224,  468, 1222,  468, 1224,  468, 1220,  468, 1202,  488, 378,  468, 1224,  468, 1222,  466, 378,  468, 376,  468, 352,
  492, 378,  468, 376,  468, 354,  490, 354,  490, 378,  466, 1226,  468, 1222,  468, 1224,  468, 1224,  468, 1224,  468, 1224,  468, 1222,  468, 1224,  468, 356,  488, 378,  466, 354,  492, 1222,
    468, 356,  490, 376,  468, 354,  490, 354,  492, 1222,  468, 378,  468, 356,  488, 1224,  468, 356,  490, 378,  466, 378,  468, 1224,  466, 378,  468, 378,  468, 1224,  468, 378,  468, 376,  468, 378,  
466, 378,  468, 378,  468, 386,  468, 1212,  468, 378,  468, 376,  468, 378,  468, 378,  466, 378,  468, 376,  468, 378,  468, 376,  468, 378,  468, 378,  468, 1222,  466, 378,  468, 378,  466, 380,  
466, 378,  466, 380,  466, 380,  466, 380,  466, 378,  466, 380,  464, 382,  464, 1226,  466, 380,  462, 384,  462, 380,  462, 384,  460, 384,  460, 388,  460, 382,  456, 388,  432, 1260,  458, 1210, 
 480, 388,  432, 412,  432, 414,  430, 416,  430, 414,  430, 414,  428, 1240,  452, 1240,  450, 394,  450, 396,  450, 396,  448, 398,  446, 400,  422, 422,  472, 372,  424, 422,  424, 420,  446, 400, 
 422, 422,  446, 398,  446, 400,  422, 422,  424, 442,  400, 444,  400, 444,  400, 444,  400, 446,  400, 446,  400, 1290,  402, 444,  400, 444,  398, 448,  396, 448,  398, 448,  396, 450,  396, 448,  
398, 446,  398, 448,  394, 450,  396, 450,  394, 450,  396, 450,  394, 452,  392, 450,  394, 454,  416, 428,  392, 452,  392, 454,  388, 456,  392, 454,  390, 458,  386, 458,  384, 460,  384, 462,  384, 462,  382, 464,  382, 460,  384, 462,  384, 462,  382, 462,  382, 462,  384, 462,  384, 484,  360, 464,  382, 486,  360, 484,  360, 486,  360, 476,  384, 470,  358, 484,  362, 484,  360, 484,  360,
 486,  360, 484,  362, 484,  358, 486,  360, 486,  360, 1332,  360, 486,  360, 486,  360, 486,  360, 486,  360, 486,  358, 488,  360, 484,  360, 486,  360, 484,  360, 486,  360, 1332,  358, 486,  360,
 486,  358, 486,  360, 486,  360, 486,  358, 486,  360, 486,  360, 486,  360, 492,  360, 480,  358, 486,  358, 488,  358, 1332,  358, 488,  358, 486,  358, 1332,  358, 1334,  358, 486,  358, 486,  358
, 488,  360};

/* ================= SYSTEM STATE ================= */
bool isLearningMode = false;
bool isAcOn = false;
int currentTemperature = 24;
bool Temp_sensor_input = false;
int lastBrightness = -1;

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

int GetBrightnessPercent() {

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
//   if (irRawLength[keyIndex] > 0) {
//     irSender.sendRaw(irRawData[keyIndex], irRawLength[keyIndex], IR_CARRIER_FREQ);
//   }
}

void sendACTemperature(int temp) {
  if (temp < 16) temp = 16;
  if (temp > 30) temp = 30;

//   uint8_t index = temp - 14; // 16→2

//   Serial.printf("Sending TEMP %d\n", temp);
//   sendIRCommand(index);
    irSender.sendRaw(rawData, irRawLength, IR_CARRIER_FREQ);
}

/* =========================================================
   AC CONTROL
   ========================================================= */
void updateACState(int brightness) {

  if (brightness < 15) {
    if (isAcOn) {
      Serial.println("AC OFF");
      irSender.sendRaw(rawData, irRawLength, IR_CARRIER_FREQ);
    //   sendIRCommand(AC_OFF_LOCATION);
      isAcOn = false;
    }
    return;
  }

  if (!isAcOn) {
    Serial.println("AC ON");
    irSender.sendRaw(rawData, irRawLength, IR_CARRIER_FREQ);
    // sendIRCommand(AC_ON_LOCATION);
    delay(2000);
    isAcOn = true;
  }

  int targetTemp = mapBrightnessToTemperature(brightness);
  Serial.printf("Brightness: %d → Temp: %d\n", brightness, targetTemp);

  if (targetTemp != currentTemperature) {
    sendACTemperature(targetTemp);
    currentTemperature = targetTemp;
  }
}

/* =========================================================
   READ LM35 SENSOR
   ========================================================= */
int read_DHT_TEMP()
{
  byte temperature = 0;
  byte humidity = 0;

  dht11.read(&temperature, &humidity, NULL);
    // int adc = analogRead(PIN_DHT_SENSOR);
    // float voltage = adc * (3.3 / 1023.0);
    // float tempC = voltage * 100.0;
    return (int)temperature;
}

/* =========================================================
   LM35 TRIGGER PROCESS
   ========================================================= */
void processLM35Mode()
{
  bool trigger = digitalRead(PIN_LM35_ENABLE);
  if (!trigger)
  {
      Temp_sensor_input = false;
      return;
  }
  if (Temp_sensor_input)
      return;
  int roomTemp = read_DHT_TEMP();
  if(roomTemp > currentTemperature)
  {
    Serial.printf("DHT_PIN Trigger -> Change temp %d\n", currentTemperature);
    sendACTemperature(currentTemperature);
    Temp_sensor_input = true;
  }
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
            // sendIRCommand(AC_ON_LOCATION);
            sendACTemperature(24);
            isAcOn = true;

            extUart.println("ON_ACK");
            Serial.println("UART: AC ON");
        
        }else if (cmd == "OFF"){
            // sendIRCommand(AC_OFF_LOCATION);
            irSender.sendRaw(rawData, irRawLength, IR_CARRIER_FREQ);  delay(200);
            isAcOn = false;

            extUart.println("OFF_ACK");
            Serial.println("UART: AC OFF");
        
        }else if (cmd.startsWith("TEMP:")){
            int temp = cmd.substring(5).toInt();
            if (temp >= 16 && temp <= 30)
            {
                sendACTemperature(temp);
                currentTemperature = temp;
                isAcOn = true;

                extUart.printf("TEMP_%d_ACK\r\n", temp);
                Serial.printf("UART: TEMP %d\n", temp);
            
            }else
                extUart.println("TEMP_INVALID");
        
        }else if (cmd == "STATUS"){
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


/* =========================================================
   SETUP
   ========================================================= */
void setup() {

// #if defined(ESP8266)
//   extUart.begin(115200);
// #elif defined(ESP32)
//   extUart.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
// #endif

  Serial.begin(115200);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_PWM_INPUT, INPUT);

  digitalWrite(PIN_STATUS_LED, HIGH);

  irReceiver.enableIRIn();
  irSender.begin();

  Serial.println("\n\n\n");
  Serial.println("System Ready");
}

/* =========================================================
   LOOP
   ========================================================= */
void loop() {

#if 0       // disable learning mode for demo
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
#endif


  static unsigned long lastCheck = 0;

  if (millis() - lastCheck > 2000)
  {
      int brightness = GetBrightnessPercent();
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
  
  // processExternalUART();
  processLM35Mode();
  
}
