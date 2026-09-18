#include <Wire.h> 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define buttonPin1 18
#define buttonPin2 19
#define buttonPin3 25
#define buttonPin4 26
#define buttonPin5 27
#define Door1     32
#define Door2     33

int count = 0;
char card_no[15];
char Maint_Card1[] = "R:abcd12345wef";
char Maint_Card2[] = "R:abcd12345hje";
char Maint_Card3[] = "R:abcd12345hjf";
char Maint_Card4[] = "R:abcd12345hjo";
char Maint_Card5[] = "R:abcd12345hjm";
char complete_feedback[13];

#define PortExp_1 0x24
#define PortExp_2 0x22
#define PortExp_3 0x20
#define PortExp_4 0x26

#define BLE_SOM_Ps 'p'
#define BLE_SOM_Pc 'P'
#define BLE_SOM_Ms 'm'
#define BLE_SOM_Mc 'M'

#define bleServerName "Wending_Machine"
#define FW_version    "2.1.3" 

BLECharacteristic *pCharacteristic = NULL;

bool deviceConnected = false;
bool RFID_ProviderFlag = false;
bool RFID_DataFlag = false;
bool Motor_RotateFlag = false;
bool Motor_ResetFlag = false;
bool Port4Active = false;
uint32_t Port4StartTime = 0;
unsigned int Motor_no = 0;
unsigned int Rotate_count = 0;
unsigned int PortA_value = 0;
unsigned int PortA_helper = 0;
unsigned int PortB_value = 0;

const uint32_t PORT4_ON_TIME = 5000UL;   // 5 seconds


#define SERVICE_UUID            "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // UART service UUID
#define CHARACTERISTIC_UUID     "beb5483e-36e1-4688-b7f7-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX  "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_CMD_SCAN_CARD "R:xxxxxx" 
