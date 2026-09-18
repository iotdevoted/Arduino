#ifndef MAIN_CONFIG_H
#define MAIN_CONFIG_H

#define AP_SSID             "IR_Controller"
#define AP_PASSWORD         "12345678"

#define WEB_SERVER_PORT     80

#define IR_RECEIVER_PIN     15
#define IR_TRANSMITTER_PIN  4

enum SystemMode
{
    MODE_CONFIG = 0,
    MODE_TEST,
    MODE_LEARNING,
    MODE_RUNNING
};

#define DEFAULT_SYSTEM_MODE MODE_CONFIG

#define SERIAL_BAUDRATE     115200

#define URL_CONFIG          "/config"
#define URL_TEST            "/test"

#define API_PROTOCOLS       "/api/protocols"
#define API_SAVE            "/api/save"
#define API_STATUS          "/api/status"
#define API_TEST_STATUS     "/api/test/status"
#define API_TEST_RESULT     "/api/test/result"

#define TEST_STEP_COUNT     4
#define TEST_TEMPERATURE_1  24
#define TEST_TEMPERATURE_2  28
#define TEST_TEMPERATURE_3  18
#define TEST_TEMPERATURE_4  24
#define TEST_DELAY_MS       1000UL


#endif