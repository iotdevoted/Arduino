
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <SimpleDHT.h>
#include "nirm_define.h"


const uint8_t g_cmdId = 1;   // In Phase1 we keep it as constant

uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

#if MODE_DEBUG
void printHexByte(uint8_t byte)
{
    if (byte < 0x10)
        Serial.print("0");

    Serial.print(byte, HEX);
}
#endif


/* ================= SYSTEM STATE ================= */

bool isAcOn_Brightness = false;
bool isAcOn_External_IN = false;
uint8_t currentTemperature = 24;
uint8_t lastBrightness = 255;

void printSavedIRData(uint16_t data[], uint16_t size, bool Case)
{
  if(!Case)
    return;
  Serial.println();
  Serial.printf("Length sending rawData_len %u, sendData_len%u\n", irRawLength_D, size);
#if 0
  Serial.print("Raw Data : ");

  for (uint16_t i = 0; i < irRawLength_D; i++)
  {
      Serial.print(data[i]);
      if (i < (irRawLength_D - 1))
          Serial.print(", ");
  }
  Serial.println();
#endif
  Serial.println("============================");
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

uint8_t calculateBrightnessPercent() {

  unsigned long highTime = pulseIn(PIN_PWM_INPUT, HIGH);
  unsigned long lowTime  = pulseIn(PIN_PWM_INPUT, LOW);
  unsigned long period = highTime + lowTime;
  if (period < 0) 
      return 15;

  float frequency = 1000000.0 / period;
  float dutyCycle = (highTime * 100.0) / period;
  uint8_t currentBrightness = (uint8_t)getBrightness(dutyCycle);
  delay(500);
  return currentBrightness;
}

/* =========================================================
   TEMP MAPPING
   ========================================================= */
uint8_t mapBrightnessToTemperature(uint8_t b) {

  if (b <= 15)  return 5;

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

void sendACTemperature(int temp) {
  if(temp == 16) {
    irSender.sendRaw(rawData_H16, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_16, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);     
  }else if(temp == 17) {
    irSender.sendRaw(rawData_H17, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_17, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
  } else if(temp == 18) {
    irSender.sendRaw(rawData_H18, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_18, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_18, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_18,irRawLength_D,false);
  } else if(temp == 19) {
    irSender.sendRaw(rawData_H19, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_19, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_19, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_19,irRawLength_D,false);
  } else if(temp == 20) {
    irSender.sendRaw(rawData_H20, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_20, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_20, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_20,irRawLength_D,false);
  } else if(temp == 21) {
    irSender.sendRaw(rawData_H21, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_21, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_21, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_21,irRawLength_D,false);
  } else if(temp == 22) {
    irSender.sendRaw(rawData_H22, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_22, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_22, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_22,irRawLength_D,false);
  } else if(temp == 23) {
    irSender.sendRaw(rawData_H23, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_23, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_23, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_23,irRawLength_D,false);
  } else if(temp == 24) {
    irSender.sendRaw(rawData_H24_ON, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_24_ON, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_24_ON, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_24_ON,irRawLength_D,false);
  } else if(temp == 25) {
    irSender.sendRaw(rawData_H25, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_25, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_25, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_25,irRawLength_D,false);
  } else if(temp == 26) {
    irSender.sendRaw(rawData_H26, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_26, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_26, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_26,irRawLength_D,false);
  } else if(temp == 27) {
    irSender.sendRaw(rawData_H27, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_27, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_27, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_27,irRawLength_D,false);
  } else if(temp == 28) {
    irSender.sendRaw(rawData_H28, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_28, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_28, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_28,irRawLength_D,false);
  } else if(temp == 29) {
    irSender.sendRaw(rawData_H29, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_29, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_29, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_29,irRawLength_D,false);
  } else if(temp == 30) {
    irSender.sendRaw(rawData_H30, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_30, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_30, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_30,irRawLength_D,false);
  }else if(temp <= 15){
    irSender.sendRaw(rawData_H24_OFF, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D1_24_OFF, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
    irSender.sendRaw(rawData_D_24_OFF, irRawLength_D, IR_CARRIER_FREQ);
    printSavedIRData(rawData_D_24_OFF,irRawLength_D,false);
  }
}

/* =========================================================
   AC CONTROL
   ========================================================= */
void updateACState(uint8_t brightness) {
  
  static uint8_t lastTemp = -1;
  if(isAcOn_External_IN)    // only work if External pin is ON
  {
    if (brightness <= 15) {
      if (isAcOn_Brightness) {
        Serial.println("Brightness is <= 15 .. Sending AC OFF Command");
        irSender.sendRaw(rawData_H24_OFF, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
        irSender.sendRaw(rawData_D1_24_OFF, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
        irSender.sendRaw(rawData_D_24_OFF, irRawLength_D, IR_CARRIER_FREQ);  delay(200);
        isAcOn_Brightness = false;
      }
      return;
    }

    if (!isAcOn_Brightness) {
      Serial.println("Brightness is > 15 .. Sending AC ON Command");
      irSender.sendRaw(rawData_H24_ON, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
      irSender.sendRaw(rawData_D1_24_ON, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
      irSender.sendRaw(rawData_D_24_ON, irRawLength_D, IR_CARRIER_FREQ);  delay(200);
      delay(2000);
      isAcOn_Brightness = true;
    }

    uint8_t targetTemp = mapBrightnessToTemperature(brightness);
    
    if (targetTemp != lastTemp) {
      Serial.printf("Brightness is %d .. Setting AC temp on %d\n", brightness, targetTemp);
      sendACTemperature(targetTemp);
      currentTemperature = targetTemp;
      lastTemp = targetTemp;
    }
  }
}

/* =========================================================
   READ LM35 SENSOR
   ========================================================= */
int read_DHT_TEMP()
{
  // read without samples.
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return -1;
  }
  return (int)temperature;
}


/* =========================================================
   LM35 TRIGGER PROCESS
   ========================================================= */
void processTempMode()
{
  bool trigger = digitalRead(PIN_LM35_ENABLE);

  // Serial.printf("External pin status: %d, AC status: %d \n",trigger, isAcOn_External_IN);
  if (trigger && (lastBrightness <=15 )){
    if (isAcOn_External_IN){
        irSender.sendRaw(rawData_H24_OFF, irRawLength_H, IR_CARRIER_FREQ);  delay(100);
        irSender.sendRaw(rawData_D1_24_OFF, irRawLength_D1, IR_CARRIER_FREQ);  delay(100);
        irSender.sendRaw(rawData_D_24_OFF, irRawLength_D, IR_CARRIER_FREQ);
        isAcOn_External_IN = false;
        Serial.printf("Turning AC OFF...\n");
      };
      return;
  }else if(trigger && (lastBrightness >15 )){
    if (!isAcOn_External_IN){
	    int roomTemp = read_DHT_TEMP();
      Serial.printf("Current room temp = %d *C\n", roomTemp);
      if( roomTemp > currentTemperature)
      {
        Serial.printf("DHT Trigger -> Set Temp %d\n", currentTemperature);
        sendACTemperature(currentTemperature);
		    isAcOn_External_IN = true;
	    }
    }
    return;
  }

  if (!trigger){
    if (isAcOn_External_IN){
      irSender.sendRaw(rawData_H24_OFF, irRawLength_H, IR_CARRIER_FREQ);  delay(100);
      irSender.sendRaw(rawData_D1_24_OFF, irRawLength_D1, IR_CARRIER_FREQ);  delay(100);
      irSender.sendRaw(rawData_D_24_OFF, irRawLength_D, IR_CARRIER_FREQ);
      isAcOn_External_IN = false;
      Serial.printf("Turning AC OFF...\n");
    }
    return;
  }
}
int getZoneCount()
{
    String response;
    Serial.println();
    Serial.println( "************ INFO ************");
    if (!sendCommand( "INFO", "{\"TS\":1793109240}", response)) // TimeStamp is just a placeholder asof now
        return 0;

    DynamicJsonDocument doc(NIRM_MAX_PAYLOAD);
    deserializeJson( doc, response );
    JsonObject data = doc["DATA"];

    Serial.println();
    Serial.print( "FW VERSION : " );
    Serial.println( data["FW_VERSION"] | "N/A" );
    Serial.print( "ML MODEL   : " );
    Serial.println( data["ML_MODEL"] | -1);
    Serial.print( "TIMESTAMP  : " );
    Serial.println( data["TS"] | 0);
    Serial.print( "ZONE COUNT : " );
    Serial.println( data["ZONE_CNT"] | 0 );
    return data["ZONE_CNT"] | 0;    // Return zone count, default to 0 if not present
}


bool getPersonCount()
{
    String response;
    Serial.println();
    Serial.println("************ COUNT ************");
    if (!sendCommand( "COUNT", "{}",response ) )
        return false;

    DynamicJsonDocument doc( NIRM_MAX_PAYLOAD );
    DeserializationError error = deserializeJson( doc, response );
    
    if (error)
    {
        Serial.println("COUNT JSON parse failed" );
        return false;
    }

    JsonArray detection = doc["DATA"]["DETECTION"];
    Serial.print( "PERSON COUNT: ");
    Serial.println( detection.size() );

    totalPeopleCount = 0;
    if(detection.size() > 0)
    {
        for (JsonArray detection : detection)
        {
            if (detection.size() < 7)
                continue;

            uint8_t zoneNumber = detection[6] | 0;
            if (zoneNumber >= 1 && zoneNumber <= MAX_ZONES)
            {
                peopleCount[zoneNumber - 1]++;
            }
        }
    }else
    {
        Serial.println("No detections found.");
        return  false;
    }

    int index = 0;
    for ( JsonArray person : detection )
    {
        Serial.println();
        Serial.print("Person ");
        Serial.println( index++ );
        Serial.print( "  Label      : ");
        Serial.println( person[0].as<const char *>() );
        Serial.print( "  Confidence : " );
        Serial.println( person[1].as<float>() );
        Serial.print( "  X          : ");
        Serial.println( person[2].as<int>() );
        Serial.print( "  Y          : " );
        Serial.println( person[3].as<int>() );
        Serial.print( "  Width      : " );
        Serial.println( person[4].as<int>() );
        Serial.print( "  Height     : " );
        Serial.println( person[5].as<int>() );
        Serial.print( "  Zone       : " );
        Serial.println( person[6].as<int>() );
    }
    
    for (int i = 0; i < MAX_ZONES; i++){
        Serial.printf("Zone %d: %d people\n", i + 1,peopleCount[i]);
        totalPeopleCount += peopleCount[i];
    }
    return true;
}

void roomManagementTask()
{
  if(isAcOn_External_IN && getPersonCount())    // only work if External pin is ON
  {
    float roomTemp = 0; // get_room_temperature();
    Serial.printf("The person count is %d, and the room temperature is %f\n", totalPeopleCount, roomTemp);
    if (totalPeopleCount == 0)
    {
      irSender.sendRaw(rawData_H24_OFF, irRawLength_H, IR_CARRIER_FREQ);  delay(200);
      irSender.sendRaw(rawData_D1_24_OFF, irRawLength_D1, IR_CARRIER_FREQ);  delay(200);
      irSender.sendRaw(rawData_D_24_OFF, irRawLength_D, IR_CARRIER_FREQ);  delay(200);
      Serial.printf("Turning AC OFF...\n");
    }
    else if (totalPeopleCount <= 4)
    {
      sendACTemperature(25);
      Serial.printf("Setting AC temperature to 25°C\n");
    }
    else
    {
        sendACTemperature(23);
        Serial.printf("Setting AC temperature to 23°C\n");
    }
  }else
    Serial.println("External pin is OFF or no people detected. AC control is disabled.");
}

void updateRelayStatus(uint8_t *zonePeopleCount)
{
    bool relayStatus[TOTAL_RELAYS] = { false };

    // Check occupancy of each configured zone
    for (uint8_t zone = 0; zone < relayMapping.totalZones; zone++)
    {
        bool zoneOccupied = (zonePeopleCount[zone] > 0);

        // Find all relays mapped to this zone
        for (uint8_t relay = 0; relay < TOTAL_RELAYS; relay++)
        {
            if (relayMapping.relayZone[relay] == zone)
            {
                relayStatus[relay] = zoneOccupied;
            }
        }
    }

    // Apply ON/OFF state to all four relays
}

void configureRelayMapping(uint8_t zoneCount)
{
    if (zoneCount < 1 || zoneCount > MAX_ZONES)
        return;

    relayMapping.totalZones = zoneCount;

    switch (zoneCount)
    {
        case 1:
            // A, B, C, D -> Zone 1
            relayMapping.relayZone[RELAY_A] = 0;
            relayMapping.relayZone[RELAY_B] = 0;
            relayMapping.relayZone[RELAY_C] = 0;
            relayMapping.relayZone[RELAY_D] = 0;
            break;

        case 2:
            // A, B -> Zone 1
            // C, D -> Zone 2
            relayMapping.relayZone[RELAY_A] = 0;
            relayMapping.relayZone[RELAY_B] = 0;
            relayMapping.relayZone[RELAY_C] = 1;
            relayMapping.relayZone[RELAY_D] = 1;
            break;

        case 3:
            // A -> Zone 1
            // B -> Zone 2
            // C, D -> Zone 3
            relayMapping.relayZone[RELAY_A] = 0;
            relayMapping.relayZone[RELAY_B] = 1;
            relayMapping.relayZone[RELAY_C] = 2;
            relayMapping.relayZone[RELAY_D] = 2;
            break;

        case 4:
            // One relay per zone
            relayMapping.relayZone[RELAY_A] = 0;
            relayMapping.relayZone[RELAY_B] = 1;
            relayMapping.relayZone[RELAY_C] = 2;
            relayMapping.relayZone[RELAY_D] = 3;
            break;
    }
}
void printTxFrame( const uint8_t *frame, size_t frameLength,
                    uint16_t payloadLength,uint16_t crc ){
    
    Serial.println("========================================");
    Serial.println("             NIRM TX FRAME");
    Serial.println("========================================");
    Serial.print("FRAME LENGTH : ");
    Serial.println(frameLength);

#if MODE_DEBUG
    Serial.println("HEX BYTES:");
    for (size_t i = 0; i < frameLength; i++)
    {
        printHexByte(frame[i]);
        Serial.print(" ");
        // New line after every 16 bytes
        if ((i + 1) % 16 == 0)
            Serial.println();
    }

    Serial.println();
    Serial.print("HEADER       : ");
    printHexByte(frame[0]);
    Serial.print(" ");
    printHexByte(frame[1]);
    Serial.println();
    Serial.print("LENGTH       : ");
    printHexByte(frame[2]);
    Serial.print(" ");
    printHexByte(frame[3]);
    Serial.println();
    Serial.print("CRC16        : ");
    printHexByte(  crc & 0xFF ); //changed to match endianess
    Serial.print(" ");
    printHexByte( (crc >> 8) & 0xFF); //changed to match endianess
    Serial.println();
#endif

    Serial.println("ASCII JSON:");
    for (uint16_t i = 0; i < payloadLength; i++)
    {
        uint8_t c = frame[4 + i];
        if (c >= 32 && c <= 126)
            Serial.write(c);
        else
            Serial.print(".");
    }
    Serial.println();
    Serial.println("========================================");
}

void flushNirmRx()
{
    while (NIRMSerial.available())
        NIRMSerial.read();
}

bool waitForNirmReady(uint32_t timeoutMs)
{
    uint32_t start = millis();
    while (millis() - start < timeoutMs)
    {
        if (digitalRead(NIRM_READY_PIN) == HIGH)
        {
            Serial.println("NIRM_READY = HIGH");
            delay(300);
            return true;
        }
        delay(10);
    }

    return false;
}

bool sendFrame(const String &json)
{
    uint16_t payloadLength = json.length();
    if (payloadLength > NIRM_MAX_PAYLOAD)
    {
        Serial.println("ERROR: Payload too large");
        return false;
    }

    uint8_t frame[NIRM_MAX_FRAME];
    size_t index = 0;
    
    frame[index++] = NIRM_HEADER_1;
    frame[index++] = NIRM_HEADER_2;
    frame[index++] = payloadLength & 0xFF;
    frame[index++] = (payloadLength >> 8) & 0xFF;
    
    memcpy( &frame[index], json.c_str(), payloadLength);
    index += payloadLength;
    
    uint16_t crc = crc16( &frame[2], 2 + payloadLength );
    frame[index++] = crc & 0xFF;
    frame[index++] = (crc >> 8)  & 0xFF;
    size_t frameLength = index;

    printTxFrame( frame, frameLength, payloadLength, crc );
    NIRMSerial.write( frame, frameLength );
    NIRMSerial.flush();

    Serial.println( ">>> COMPLETE FRAME SENT TO NIRM");
    return true;
}

bool receiveFrame( String &json, uint32_t timeoutMs)
{
    uint32_t start = millis();
    bool headerFound = false;
    
    while (millis() - start < timeoutMs)
    {
        if (NIRMSerial.available())
        {
            uint8_t byte1 = NIRMSerial.read();
            if (byte1 == NIRM_HEADER_1)
            {
                uint32_t headerTimeout = millis();
                while (millis() - headerTimeout < 100)
                {
                    if (NIRMSerial.available())
                    {
                        uint8_t byte2 = NIRMSerial.read();
                        if (byte2 == NIRM_HEADER_2)
                            headerFound = true;
                        break;
                    }
                    delay(1);
                }

                if (headerFound)
                    break;
            }
        }
    }


    if (!headerFound)
    {
        Serial.println( "ERROR: RX header timeout" );
        return false;
    }

    uint8_t lengthBytes[2];
    uint8_t receivedLengthBytes = 0;
    uint32_t lengthStart = millis();

    while (receivedLengthBytes < 2)
    {
        if (NIRMSerial.available())
        {
            lengthBytes[receivedLengthBytes++] = NIRMSerial.read();
        }

        if (millis() - lengthStart > timeoutMs)
        {
            Serial.println("ERROR: RX length timeout");
            return false;
        }
    }

    uint16_t payloadLength = (uint16_t)lengthBytes[0] | ((uint16_t)lengthBytes[1] << 8);

    if (payloadLength > NIRM_MAX_PAYLOAD)
    {
        Serial.print("ERROR: Invalid RX payload length: ");
        Serial.println(payloadLength);
        return false;
    }

    uint8_t *payload =  new uint8_t[payloadLength + 1];
    uint16_t receivedPayload = 0;
    uint32_t payloadStart = millis();

    while (receivedPayload < payloadLength)
    {
        if (NIRMSerial.available())
            payload[receivedPayload++] = NIRMSerial.read();

        if (millis() - payloadStart > timeoutMs)
        {
            delete[] payload;
            Serial.println( "ERROR: RX payload timeout");
            return false;
        }
    }

    payload[payloadLength] = '\0';
    uint8_t crcBytes[2];
    uint8_t receivedCRCBytes = 0;
    uint32_t crcStart = millis();

    while (receivedCRCBytes < 2)
    {
        if (NIRMSerial.available())
            crcBytes[ receivedCRCBytes++] = NIRMSerial.read();

        if (millis() - crcStart > timeoutMs)
        {
            delete[] payload;
            Serial.println("ERROR: RX CRC timeout");
            return false;
        }
    }

    uint16_t receivedCRC = (uint16_t)crcBytes[0] | ((uint16_t)crcBytes[1] << 8);
    uint8_t *crcBuffer = new uint8_t[2 + payloadLength];
    crcBuffer[0] = lengthBytes[0];
    crcBuffer[1] = lengthBytes[1];
    memcpy( &crcBuffer[2], payload, payloadLength);

    uint16_t calculatedCRC = crc16( crcBuffer, 2 + payloadLength);
    delete[] crcBuffer;

    Serial.println();
    Serial.println("========================================");
    Serial.println("             NIRM RX FRAME");
    Serial.println("========================================");
#if MODE_DEBUG
    Serial.print("HEADER       : AA 55");
    Serial.println();
    Serial.print("LENGTH       : ");
    printHexByte(lengthBytes[0]);
    Serial.print(" ");
    printHexByte(lengthBytes[1]);
    Serial.println();
    Serial.print("PAYLOAD      : ");
    for (uint16_t i = 0; i < payloadLength; i++)
    {
        printHexByte(payload[i]);
        Serial.print(" ");
    }
    Serial.println();
    Serial.print("RX CRC       : ");
    printHexByte(crcBytes[0]);
    Serial.print(" ");
    printHexByte(crcBytes[1]);
    Serial.println();

    Serial.print("CALC CRC     : ");
    printHexByte(  calculatedCRC & 0xFF ); //changed to match endianess
    Serial.print(" ");
    printHexByte( (calculatedCRC >> 8) & 0xFF ); //changed to match endianess
    Serial.println();
    Serial.println();
#endif
    Serial.print("JSON: ");
    Serial.println((char *)payload);
    Serial.println( "========================================");

    if (receivedCRC != calculatedCRC)
    {
        Serial.println( "ERROR: RX CRC MISMATCH");
        delete[] payload;
        return false;
    }

    Serial.println( "RX CRC: OK");
    json = String( (char *)payload);
    delete[] payload;
    return true;
}
bool sendCommand( const char *command, const String &dataJson, String &response )
{
    if (digitalRead(NIRM_READY_PIN) != HIGH)
    {
        Serial.println( "ERROR: NIRM_READY is LOW");
        return false;
    }

    String json;
    json.reserve(512);
        json = "{";
        json += "\"CMD_ID\":";
        json += g_cmdId;
        json += ",\"CMD\":\"";
        json += command;
        json += "\"";
        json += ",\"DATA\":";
        json += dataJson;
        json += "}";

    flushNirmRx();
    
    if (!sendFrame(json))
        return false;

    if ( !receiveFrame( response, NIRM_TIMEOUT_MS) )
    {
        Serial.println("ERROR: No valid response" );
        return false;
    }

    DynamicJsonDocument doc(NIRM_MAX_PAYLOAD);
    DeserializationError error = deserializeJson( doc, response );
    if (error)
    {
        Serial.print( "JSON parse error: " );
        Serial.println( error.c_str() );
        return false;
    }

    uint16_t responseCmdId = doc["CMD_ID"] | 0;
    int statusCode = doc["STATUS"]["CODE"] | -1;
    const char *statusText = doc["STATUS"]["TXT"] | "";
#if MODE_DEBUG    
    Serial.println();
    Serial.println( "========== RESPONSE ==========" );
    Serial.print( "CMD_ID     : ");
    Serial.println( responseCmdId);
    Serial.print( "STATUS CODE: ");
    Serial.println(statusCode );

    Serial.print("STATUS TXT : ");
    Serial.println(statusText);
    Serial.println( "==============================");
#endif
    if (responseCmdId != g_cmdId)
    {
        Serial.println( "WARNING: CMD_ID mismatch");
        return false;
    }
    return statusCode == 1;
}

void setup() {

  NIRMSerial.begin( NIRM_BAUDRATE,
                  SERIAL_8N1,
                  NIRM_UART_RX_PIN,
                  NIRM_UART_TX_PIN
                );

  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_PWM_INPUT, INPUT);
  pinMode(PIN_LM35_ENABLE, INPUT);

  digitalWrite(PIN_STATUS_LED, HIGH);
  irSender.begin();

  Serial.println("\n\n\n");
  Serial.println("System Ready");
}

void loop() {

    static unsigned long lastBrightnessCheck = 0;
    static unsigned long lastTempModeCheck = 0;
    static unsigned long lastOccupancyCheck = 0;

    unsigned long currentMillis = millis();

    // Every 2 seconds: Check brightness and update AC
    if (currentMillis - lastBrightnessCheck >= 2000UL)
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
      lastBrightnessCheck = currentMillis;
    } 

    // Every 5 seconds: Process temperature mode
    if (currentMillis - lastTempModeCheck >= 5000UL)
    {
        processTempMode();
        lastTempModeCheck = currentMillis;
    }

    // Every 10 seconds: Update room occupancy
    if (currentMillis - lastOccupancyCheck >= 10000UL)
    {
        updateRelayStatus(peopleCount);
        lastOccupancyCheck = currentMillis;
    }

}