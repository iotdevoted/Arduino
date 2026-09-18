#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>

#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>

#include "main_config.h"
#include "config_page.html"
#include "test_page.html"

WebServer server( WEB_SERVER_PORT );
IRac irac( IR_TRANSMITTER_PIN );
SystemMode currentMode = DEFAULT_SYSTEM_MODE;
decode_type_t selectedProtocol = decode_type_t::UNKNOWN;

uint8_t testStep = 0;
bool testRunning = false;
bool testComplete = false;
int currentTemperature = 0;
bool testResultSelected = false;
unsigned long testStepStartTime = 0;
unsigned long testRemainingTime = 0;

String currentCommand = "Waiting...";

String testStatus = "TEST STARTING...";

void handleStatus();
void startTestMode();
void handleTestPage();
void handleNotFound();
void startConfigMode();
void runTestSequence();
void handleProtocols();
void handleConfigPage();
void handleTestStatus();
void handleTestResult();
void handleSaveProtocol();
String getProtocolName( decode_type_t protocol );
void sendTestCommand( bool power, uint8_t temperature );


void setup(){
    Serial.begin( SERIAL_BAUDRATE );
    delay(100);
    startConfigMode();
}


void loop(){
    server.handleClient();
    if ( currentMode == MODE_TEST )
        runTestSequence();

    delay(2);
}


void startConfigMode(){
    currentMode = MODE_CONFIG;
    WiFi.mode( WIFI_AP );

    if (!WiFi.softAP( AP_SSID, AP_PASSWORD))
    {
        Serial.println( "ERROR: Failed to start WiFi AP" );
        return;
    }

    IPAddress ip = WiFi.softAPIP();
    Serial.print("  SSID    : "); Serial.println( AP_SSID );
    Serial.print("Password  : "); Serial.println( AP_PASSWORD);
    Serial.print("IP Add    : "); Serial.println( ip );

    server.on( URL_CONFIG, HTTP_GET, handleConfigPage );
    server.on( URL_TEST, HTTP_GET, handleTestPage );
    server.on( API_PROTOCOLS, HTTP_GET, handleProtocols );
    server.on( API_SAVE, HTTP_POST, handleSaveProtocol );
    server.on( API_STATUS, HTTP_GET, handleStatus );
    server.on( API_TEST_STATUS, HTTP_GET, handleTestStatus );
    server.on( API_TEST_RESULT, HTTP_POST, handleTestResult );
    server.onNotFound( handleNotFound );

    server.begin();
}


void handleConfigPage(){
    server.send( 200, "text/html", CONFIG_HTML );
}


void handleTestPage(){
    if (currentMode != MODE_TEST){
        server.send( 403, "text/plain", "System is not in TEST MODE");
        return;
    }

    server.send( 200, "text/html", TEST_HTML );
}


String getProtocolName( decode_type_t protocol){
    return typeToString( protocol, false );
}


void handleProtocols(){
    String json =   "[";
    bool first =    true;

    for (int i=1; i<=(int)decode_type_t::kLastDecodeType;i++){
        decode_type_t protocol = static_cast<decode_type_t>(i);
        String name = getProtocolName(protocol);

        if ( name.length() == 0 || name == "UNKNOWN" || name == "UNUSED")
            continue;

        if (!first)         json += ",";

        first = false;
        json += "{";
        json += "\"id\":" + String(i);
        json += ",";
        json += "\"name\":\"";
            name.replace( "\\", "\\\\" );
            name.replace( "\"", "\\\"" );
        json += name;
        json += "\"";
        json += "}";
    }

    json += "]";
    server.send( 200, "application/json", json );
}


void handleSaveProtocol(){

    if ( !server.hasArg("protocol")){
        server.send(400, "application/json",
            "{\"success\":false,\"message\":\"Protocol not selected\"}"
        );
        return;
    }

    int protocolNumber = server.arg( "protocol" ).toInt();
    if ( protocolNumber <= 0 || protocolNumber > (int)decode_type_t::kLastDecodeType ){
        server.send( 400, "application/json",
            "{\"success\":false,\"message\":\"Invalid protocol\"}"
        );
        return;
    }

    selectedProtocol = static_cast<decode_type_t>( protocolNumber );
    String protocolName = getProtocolName( selectedProtocol );
    delay(2000);
    startTestMode();

    String response;
    response += "{";
    response += "\"success\":true,";
    response += "\"id\":" + String(protocolNumber);
    response += ",";
    response += "\"name\":\"";
    response += protocolName;
    response += "\"";
    response += "}";

    server.send( 200, "application/json", response );
}


void startTestMode(){
    currentMode = MODE_TEST;
    testStep = 0;
    testRunning = true;
    testComplete = false;
    testRemainingTime = 0;
    currentTemperature = 0;
    testResultSelected = false;
    testStepStartTime = millis();
    currentCommand = "Starting...";
    testStatus = "TEST STARTING...";
}


void runTestSequence(){
    unsigned long now = millis();

    if (!testRunning)
        return;

    if (testStep == 0){
        currentTemperature = TEST_TEMPERATURE_1;
        currentCommand = "AC ON";
        testStatus = "Sending AC ON at 24°C";

        Serial.println();
        Serial.println( "[TEST 1]" );
        Serial.println(testStatus);
        sendTestCommand( true, TEST_TEMPERATURE_1 );

        testStep =  1;
        testStepStartTime = now;
        return;
    }
    if ( testStep == 1 && now - testStepStartTime >= TEST_DELAY_MS ){
        currentTemperature = TEST_TEMPERATURE_2;
        currentCommand = "AC ON";
        testStatus = "Sending AC ON at 28°C";

        Serial.println();
        Serial.println( "[TEST 2]" );
        Serial.println( testStatus );
        sendTestCommand( true, TEST_TEMPERATURE_2 );

        testStep = 2;
        testStepStartTime = now;
        return;
    }
    if ( testStep == 2 && now - testStepStartTime >= TEST_DELAY_MS){
        currentTemperature = TEST_TEMPERATURE_3;
        currentCommand = "AC ON";
        testStatus = "Sending AC ON at 18°C";

        Serial.println();
        Serial.println( "[TEST 3]" );
        Serial.println( testStatus );
        sendTestCommand( true, TEST_TEMPERATURE_3 );

        testStep = 3;
        testStepStartTime = now;
        return;
    }
    if ( testStep == 3 && now - testStepStartTime >= TEST_DELAY_MS ){
        currentTemperature = TEST_TEMPERATURE_4;
        currentCommand = "AC OFF";
        testStatus = "Sending AC OFF at 24°C";

        Serial.println( "[TEST 4]" );
        Serial.println( testStatus );
        sendTestCommand( false, TEST_TEMPERATURE_4 );
        testStep = 4;
        testStepStartTime = now;
        return;
    }
    if ( testStep == 4 && now - testStepStartTime >= 1000 ){
        testRunning = false;
        testComplete = true;
        testResultSelected = false;
        currentCommand = "TEST COMPLETE";
        testStatus = "TEST COMPLETE - WAITING FOR USER";
        testRemainingTime = 0;
        delay(3000);
    }
    if ( testRunning && testStep > 0 && testStep < 4 ){
        unsigned long elapsed =now - testStepStartTime;
        if ( elapsed < TEST_DELAY_MS ) {
            testRemainingTime = TEST_DELAY_MS - elapsed;
        }
    }
}


void sendTestCommand( bool power, uint8_t temperature){
    
    stdAc::state_t state;
    state.protocol      = selectedProtocol;
    state.power         = power;
    state.degrees       = temperature;
    state.celsius       = true;
    state.mode          = stdAc::opmode_t::kCool;
    state.fanspeed      = stdAc::fanspeed_t::kAuto;
    state.swingv        = stdAc::swingv_t::kOff;
    state.swingh        = stdAc::swingh_t::kOff;
    state.quiet         = false;
    state.turbo         = false;
    state.econo         = false;
    state.light         = false;
    state.filter        = false;
    state.clean         = false;
    state.beep          = false;
    state.sleep         =   -1;

    irac.sendAc( state );
    // Serial.println( "IR command sent." );
}


void handleStatus(){
    String response;
    response += "{";
    response += "\"mode\":" + String((int)currentMode);
    response += ",";
    response += "\"protocol_id\":" + String((int)selectedProtocol );
    response += ",";
    response += "\"protocol_name\":\"";
    response += getProtocolName( selectedProtocol );
    response += "\"";
    response += "}";

    server.send( 200, "application/json", response );
}


void handleTestStatus(){
    unsigned long remaining = 0;

    if (testRunning && testStep > 0 && testStep < 4 ){
        unsigned long elapsed = millis() - testStepStartTime;

        if (elapsed < TEST_DELAY_MS )
            remaining = TEST_DELAY_MS - elapsed; 
    }

    String response;
    response += "{";
    response += "\"mode\":" + String((int)currentMode);
    response += ",";
    response += "\"protocol_name\":\"";
    response += getProtocolName( selectedProtocol );
    response += "\",";
    response += "\"temperature\":" + String( currentTemperature );
    response += ",";
    response += "\"command\":\"";
    response += currentCommand;
    response += "\",";
    response += "\"step\":" + String( testStep );
    response += ",";
    response += "\"total_steps\":" + String( TEST_STEP_COUNT );
    response += ",";
    response += "\"remaining_ms\":" + String( remaining );
    response += ",";
    response += "\"running\":" + String( testRunning ? "true" : "false" );
    response += ",";
    response += "\"complete\":" + String( testComplete ? "true" : "false" );
    response += ",";
    response += "\"result_selected\":" + String( testResultSelected ? "true" : "false");
    response += ",";
    response += "\"status\":\"";
    response += testStatus;
    response += "\"";
    response += "}";

    server.send( 200, "application/json", response );
}

void handleNotFound() {
    server.send( 404, "text/plain", "404 - Page Not Found" );
}


void handleTestResult(){
    if (!testComplete){
        server.send( 400, "application/json",
            "{\"success\":false,\"message\":\"Test is not completed yet\"}"
        );
        return;
    }
    if (testResultSelected){
        server.send( 400, "application/json",
            "{\"success\":false,\"message\":\"Result already selected\"}"
        );
        return;
    }
    if (!server.hasArg( "result" ) ) {
        server.send( 400, "application/json",
            "{\"success\":false,\"message\":\"Result not provided\"}"
        );
        return;
    }

    String result = server.arg( "result" );
    result.toLowerCase();
    testResultSelected = true;

    if (result == "yes"){
        currentMode = MODE_RUNNING;
        testStatus = "AC TEST PASSED - RUNNING MODE";
        currentCommand = "TEST PASSED";
        server.send( 200,"application/json",
            "{"
                "\"success\":true,"
                "\"mode\":\"running\","
                "\"message\":\"AC test passed. System switched to RUNNING MODE.\""
            "}"
        );
        return;
    }
    if (result == "no"){
        currentMode = MODE_LEARNING;
        testStatus = "AC TEST FAILED - LEARNING MODE";
        currentCommand = "LEARNING MODE";

        server.send( 200, "application/json",
            "{"
                "\"success\":true,"
                "\"mode\":\"learning\","
                "\"message\":\"AC test failed. System switched to LEARNING MODE.\""
            "}"
        );
        return;
    }

    testResultSelected = false;
    server.send( 400, "application/json",
        "{"
            "\"success\":false,"
            "\"message\":\"Invalid result. Use yes or no.\""
        "}"
    );
}