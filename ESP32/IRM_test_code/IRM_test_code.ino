#include <Arduino.h>
#include <ArduinoJson.h>

#define NIRM_BAUDRATE       115200
#define NIRM_UART_RX_PIN    16
#define NIRM_UART_TX_PIN    17
#define NIRM_READY_PIN      4

#define MODE_DEBUG 0
#define NIRM_HEADER_1      0x55 // using little-endian for header to match the CRC16 calculation
#define NIRM_HEADER_2      0xAA // using little-endian for header to match the CRC16 calculation
#define NIRM_MAX_PAYLOAD   12000
#define NIRM_MAX_FRAME     (2 + 2 + NIRM_MAX_PAYLOAD + 2)
#define NIRM_TIMEOUT_MS    5000

HardwareSerial NIRMSerial(2);

// ============================================================
// Global Command ID
// ============================================================
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


// ============================================================
// Print complete UART frame
// ============================================================

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

    Serial.println("ASCII JSON: ");
    for (uint16_t i = 0; i < payloadLength; i++)
    {
        uint8_t c = frame[4 + i];
        if (c >= 32 && c <= 126)
            Serial.write(c);
        else
            Serial.print(".");
    }
    Serial.println("========================================");
    Serial.println();
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


// ============================================================
// INFO TEST
// ============================================================
void testInfo()
{
    String response;
    Serial.println();
    Serial.println( "************ INFO ************");
    if (!sendCommand( "INFO", "{\"TS\":1793109240}", response)) // TimeStamp is just a placeholder asof now
        return;

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
}
// ============================================================
// COUNT
// ============================================================
void testCount()
{
    String response;
    Serial.println();
    Serial.println("************ COUNT ************");
    if (!sendCommand( "COUNT", "{}",response ) )
        return;

    DynamicJsonDocument doc( NIRM_MAX_PAYLOAD );
    DeserializationError error = deserializeJson( doc, response );

    if (error)
    {
        Serial.println("COUNT JSON parse failed" );
        return;
    }

    JsonArray detection = doc["DATA"]["DETECTION"];
    Serial.print( "PERSON COUNT: ");
    Serial.println( detection.size() );

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
}

void testSetZone()
{
    String response;
    Serial.println();
    Serial.println("************ SETZONE ************");
    String data = R"JSON( {
            "ZONE_COUNT": 4,
            "ZONES": [
                [[0,0],[0,31],[40,31],[40,0]],
                [[40,0],[40,31],[80,31],[80,0]],
                [[0,31],[0,62],[40,62],[40,31]],
                [[40,31],[40,62],[80,62],[80,31]]
            ]
            }
            )JSON";
    sendCommand( "SETZONE", data, response );
}

void testImage()
{
    String response;

    Serial.println();
    Serial.println("========================================");
    Serial.println("Sending GET IMAGE command");
    Serial.println("========================================");

    sendCommand(
        "IMG",
        "{\"CMD_ID\":1,\"CMD\":\"IMG\",\"DATA\":{}}",
        response
    );

    Serial.println();
    Serial.println("IMG Response:");
    Serial.println(response);
}

void printMenu()
{
    Serial.println();
    Serial.println( "================================" );
    Serial.println( "       NIRM HOST TEST MENU      " );
    Serial.println( "================================" );

    Serial.println("i = INFO" );
    Serial.println("c = COUNT" );
    Serial.println("g = IMG" );
    Serial.println("z = SETZONE" );
    Serial.println("r = Check READY");
    Serial.println("x = Clear RX");
    Serial.println("================================");
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println( "================================" );
    Serial.println( "       ESP32 NIRM HOST          " );
    Serial.println( "================================");

    pinMode( NIRM_READY_PIN, INPUT );
    NIRMSerial.begin(
        NIRM_BAUDRATE,
        SERIAL_8N1,
        NIRM_UART_RX_PIN,
        NIRM_UART_TX_PIN
    );


    Serial.println( "NIRM UART initialized" );
    Serial.println( "Waiting for NIRM_READY...");
    if (waitForNirmReady(10000))
        Serial.println( "NIRM READY" );
    else
        Serial.println("ERROR: NIRM READY timeout");
    printMenu();
}

String serialCommand = "";

void loop()
{
    while (Serial.available())
    {
        char receivedChar = Serial.read();

        // Enter key received
        if (receivedChar == '\n' || receivedChar == '\r')
        {
            // Ignore empty command
            if (serialCommand.length() > 0)
            {
#if MODE_DEBUG
                Serial.print("Command received: ");
                Serial.println(serialCommand);
#endif
                // Use the first character for switch-case
                char command = serialCommand.charAt(0);

                switch (command)
                {
                    case 'i':
                    case 'I':
                        testInfo();
                        break;

                    case 'c':
                    case 'C':
                        testCount();
                        break;

                    case 'g':
                    case 'G':
                        testImage();
                        break;

                    case 'z':
                    case 'Z':
                        testSetZone();
                        break;

                    case 'r':
                    case 'R':
                        Serial.print("NIRM_READY = ");
                        Serial.println(
                            digitalRead(NIRM_READY_PIN) ? "HIGH" : "LOW"
                        );
                        break;

                    case 'x':
                    case 'X':
                        flushNirmRx();
                        Serial.println("RX buffer cleared");
                        break;

                    default:
                        Serial.println("Invalid command!");
                        break;
                }
                serialCommand = "";
                printMenu();
            }
        }
        else
            serialCommand += receivedChar;
    }
}
