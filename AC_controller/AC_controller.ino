#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define firmwareVersion "0.0.2" // Demo //Learning disabled
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
// #define EEPROM_TOTAL_SIZE     4096
#define IR_RAW_BUFFER_SIZE    750   // Reduced for EEPROM fit
#define IR_CARRIER_FREQ       38
#define AC_OFF_LOCATION       0
#define AC_ON_LOCATION        1
#define MIN_IR_FRAME_LENGTH   100

// #define BUTTON_LONG_PRESS_MS  3000UL
// #define IR_LEARN_TIMEOUT_MS   20000UL

#define TOTAL_IR_KEYS         18   // ON, OFF, 16→30
// #define LEARNING_SESSION_TIMEOUT_MS 150000UL   // 2 minutes
#define BRIGHTNESS_CHANGE_THRESHOLD 3

SoftwareSerial extUart(UART_RX_PIN, UART_TX_PIN);
/* ================= IR OBJECTS ================= */
IRrecv irReceiver(PIN_IR_RECEIVER, IR_RAW_BUFFER_SIZE, 15, true);
IRsend irSender(PIN_IR_SENDER);
decode_results irResults;

/* ================= IR STORAGE ================= */
// uint16_t irRawData[IR_RAW_BUFFER_SIZE];
uint8_t irRawLength = 185;

uint16_t rawData_16[185] = {1022, 532,  1020, 532,  1022, 2560,  1022, 2534,  998, 534,  1020, 556,  998, 2558,  1000, 2558,  998, 556,  998, 556,  998
, 2560,  1000, 556,  998, 2558,  1000, 530,  1022, 530,  1022, 554,  1000, 2556,  1002, 532,  1020, 554,  1000, 528,  1026, 552,  1000, 530,  1024, 2558,  1000, 554,  1000, 528,  1024, 554,  1000, 554
,  998, 2558,  1002, 528,  1024, 530,  1024, 554,  998, 530,  1024, 2558,  1000, 528,  1024, 530,  1022, 530,  1024, 554,  1000, 528,  1024, 554,  1000, 530,  1022, 2558,  1000, 528,  1026, 528,  1024
, 530,  1022, 530,  1024, 530,  1024, 528,  1026, 528,  1024, 528,  1024, 530,  1024, 528,  1028, 526,  1026, 526,  1026, 526,  1026, 528,  1026, 526,  1028, 524,  1028, 526,  1028, 526,  1028, 526,  
1026, 528,  1026, 2530,  1028, 2530,  1030, 524,  1028, 524,  1030, 546,  1030, 522,  1030, 524,  1030, 522,  1028, 2506,  1054, 522,  1028, 2506,  1052, 526,  1028, 526,  1026, 526,  1002, 528,  1048
, 504,  1026, 2530,  1026, 2534,  1024, 2534,  1024, 8944,  680, 6518,  680, 3668,  682, 6518,  680, 6520,  678, 6520,  678, 3672,  676, 3692,  658, 3692,  658, 6542,  656, 3694,  656, 3694,  656};

uint16_t rawData_17[185] = {1038, 556,  998, 554,  1000, 2558,  998, 2560,  1000, 554,  1000, 554,  1000, 2558,  1000, 2558,  1000, 530,  1030, 548,  
1000, 2558,  1000, 554,  1024, 2534,  1000, 528,  1048, 506,  1022, 532,  1022, 2558,  998, 530,  1024, 554,  1000, 554,  998, 554,  1000, 552,  1000, 2560,  998, 554,  998, 532,  1022, 554,  1000,
 552,  1000, 2558,  1000, 530,  1022, 554,  1000, 534,  1020, 2558,  1000, 2560,  1000, 554,  998, 530,  1022, 532,  1024, 528,  1022, 532,  1022, 532,  1020, 532,  1022, 2560,  998, 554,  980, 550,
   1002, 552,  1000, 552,  1002, 552,  1022, 532,  1020, 532,  1002, 554,  1020, 554,  998, 532,  1002, 550,  1002, 550,  1002, 554,  1000, 552,  1002, 550,  1004, 550,  1004, 550,  1024, 530,  1002, 552,  
1024, 528,  1026, 2556,  980, 2578,  1004, 526,  1024, 528,  1028, 526,  1028, 526,  1028, 526,  1028, 528,  1028, 2554,  1028, 502,  1052, 2530,  1032, 522,  1032, 520,  1034, 518,  1036, 518,  1034,
 520,  1034, 2500,  1056, 2500,  1058, 520,  1032, 8912,  716, 6484,  714, 3636,  714, 6484,  712, 6488,  710, 6488,  708, 3642,  708, 6492,  706, 3644,  682, 6516,  680, 3670,  682, 3670,  680};

uint16_t rawData_18[185] = {1078, 514,  1064, 488,  1064, 2494,  1040, 2518,  1066, 488,  1038, 492,  1060, 2520,  1038, 2520,  1064, 490,  1066, 464,
  1088, 2494,  1066, 464,  1088, 2494,  1066, 488,  1064, 464,  1090, 464,  1090, 2490,  1068, 486,  1038, 514,  1066, 466,  1088, 464,  1090, 486,  1066, 2492,  1064, 490,  1064, 466,  1088, 464,
  1088, 488,  1066, 2492,  1068, 464,  1088, 464,  1088, 2492,  1068, 464,  1090, 2490,  1068, 462,  1090, 486,  1068, 464,  1088, 464,  1090, 488,  1068, 486,  1066, 464,  1088, 2492,  1068, 462,
  1090, 
462,  1090, 486,  1068, 486,  1066, 486,  1066, 464,  1090, 464,  1088, 464,  1090, 464,  1090, 486,  1066, 488,  1066, 464,  1088, 464,  1090, 486,  1066, 466,  1088, 464,  1088, 464,  1090, 464,  1088, 464,  1088, 464,  1088, 2492,  1066, 2492,  1066, 466,  1088, 486,  1066, 486,  1068, 486,  1068, 486,  1068, 486,  1068, 2470,  1088, 486,  1068, 2468,  1090, 486,  1066, 486,  1066, 488,  1066, 
488,  1064, 490,  1062, 2472,  1084, 492,  1058, 2476,  1056, 8912,  712, 6486,  712, 3640,  708, 6490,  708, 6490,  706, 6492,  682, 3666,  682, 6518,  680, 3670,  680, 6520,  678, 3672,  678, 3690, 
 658};

 uint16_t rawData_19[185] = {1090, 460,  1092, 464,  1088, 2492,  1066, 2492,  1066, 466,  1088, 466,  1088, 2492,  1064, 2492,  1068, 486,  1068, 486,
  1066, 2490,  1068, 466,  1086, 2492,  1068, 466,  1084, 490,  1066, 466,  1090, 2490,  1068, 464,  1088, 488,  1066, 466,  1090, 464,  1088, 486,  1066, 2490,  1068, 464,  1090, 484,  1068, 486,  1068, 462,  1090, 2490,  1068, 486,  1068, 486,  1068, 2490,  1068, 2490,  1068, 2490,  1068, 464,  1090, 486,  1068, 462,  1092, 462,  1090, 464,  1090, 464,  1090, 462,  1090, 2490,  1068, 486,  1066,
 462,  1092, 464,  1090, 464,  1090, 462,  1090, 464,  1090, 464,  1090, 486,  1068, 486,  1068, 464,  1090, 486,  1068, 486,  1068, 486,  1068, 484,  1068, 484,  1068, 486,  1066, 486,  1068, 486,
   1068, 486,  1066, 488,  1066, 2470,  1088, 2470,  1088, 488,  1064, 490,  1062, 492,  1038, 516,  1034, 518,  1032, 522,  1030, 2504,  1052, 502,  1050, 2508,  1050, 504,  1048, 504,  1024, 530,  1024,
 530,  1024, 530,  1024, 2536,  1022, 532,  1020, 552,  1000, 8968,  660, 6540,  658, 3692,  656, 6544,  658, 6542,  656, 6546,  654, 3694,  654, 6546,  654, 3696,  652, 6548,  650, 3698,  652, 3702, 
 648};

 uint16_t rawData_20[185] = {1108, 484,  1070, 484,  1068, 2488,  1070, 2488,  1070, 464,  1090, 484,  1070, 2488,  1070, 2488,  1070, 486,  1068, 482,
  1070, 2488,  1070, 484,  1070, 2488,  1070, 484,  1068, 484,  1070, 484,  1070, 2488,  1070, 484,  1070, 484,  1068, 484,  1070, 460,  1092, 462,  1092, 2490,  1070, 484,  1070, 462,  1092, 484,
    1068, 462,  1090, 2488,  1070, 484,  1070, 2490,  1070, 484,  1068, 458,  1094, 2488,  1070, 484,  1070, 460,  1092, 486,  1068, 484,  1068, 486,  1068, 484,  1068, 462,  1090, 2490,  1068, 484,  1070, 
462,  1090, 484,  1068, 484,  1070, 462,  1092, 462,  1090, 484,  1070, 484,  1070, 460,  1092, 486,  1068, 484,  1068, 484,  1070, 484,  1068, 460,  1092, 484,  1070, 484,  1068, 484,  1070, 484,
  1068, 484,  1068, 484,  1068, 2490,  1068, 2490,  1068, 486,  1068, 484,  1068, 462,  1090, 462,  1092, 486,  1068, 464,  1088, 2490,  1068, 486,  1068, 2490,  1068, 486,  1066, 486,  1068, 484,  1068, 
484,  1068, 486,  1068, 486,  1066, 2468,  1090, 2470,  1088, 8878,  748, 6452,  746, 3606,  746, 6452,  744, 6456,  742, 6458,  738, 3612,  714, 6486,  714, 3636,  712, 6486,  710, 3640,  708, 3642, 
 708};

 uint16_t rawData_21[185] = {1092, 462,  1092, 484,  1070, 2488,  1070, 2488,  1070, 484,  1070, 484,  1070, 2490,  1068, 2488,  1070, 484,  1070, 462,
  1090, 2488,  1068, 484,  1070, 2488,  1070, 484,  1070, 464,  1090, 484,  1068, 2490,  1070, 464,  1090, 486,  1068, 462,  1090, 486,  1068, 462,  1092, 2488,  1070, 484,  1068, 486,  1068, 484,
    1070, 462,  1092, 2490,  1070, 484,  1068, 2490,  1068, 464,  1090, 2490,  1070, 2488,  1070, 464,  1088, 484,  1070, 484,  1068, 462,  1092, 484,  1068, 486,  1068, 486,  1068, 2490,  1068, 462,  1092,
 484,  1068, 462,  1090, 464,  1090, 464,  1088, 486,  1068, 464,  1090, 486,  1068, 484,  1070, 484,  1070, 462,  1092, 484,  1068, 462,  1092, 464,  1090, 484,  1068, 486,  1068, 486,  1068, 484,
   1070, 460,  1092, 462,  1092, 2488,  1070, 2488,  1070, 462,  1092, 462,  1090, 486,  1066, 486,  1068, 486,  1068, 486,  1068, 2490,  1068, 484,  1068, 2490,  1068, 486,  1068, 484,  1068, 486,  1066,
 486,  1068, 486,  1066, 486,  1066, 2470,  1088, 488,  1064, 8882,  744, 6456,  742, 3608,  742, 6458,  714, 6484,  714, 6486,  712, 3638,  710, 6490,  708, 3642,  706, 6494,  704, 3646,  682, 3668, 
 682};

 uint16_t rawData_22[185] = {1108, 484,  1068, 462,  1092, 2490,  1070, 2488,  1070, 484,  1068, 486,  1068, 2488,  1070, 2488,  1070, 484,  1068, 486,
  1068, 2490,  1070, 462,  1090, 2488,  1070, 460,  1092, 486,  1068, 484,  1068, 2490,  1070, 484,  1068, 484,  1070, 462,  1092, 464,  1090, 484,  1068, 2490,  1070, 462,  1090, 484,  1068, 462,
    1092, 462,  1092, 2488,  1070, 484,  1068, 2488,  1070, 2488,  1070, 484,  1068, 2488,  1070, 484,  1068, 462,  1092, 484,  1068, 486,  1068, 484,  1068, 484,  1070, 462,  1090, 2488,  1070, 462,  1090,
 460,  1094, 484,  1068, 462,  1092, 484,  1070, 484,  1068, 484,  1068, 464,  1090, 462,  1090, 484,  1070, 484,  1068, 484,  1070, 460,  1092, 462,  1090, 464,  1090, 462,  1090, 484,  1068, 462,
   1092, 462,  1092, 462,  1090, 2490,  1068, 2490,  1068, 462,  1090, 486,  1068, 486,  1068, 486,  1068, 462,  1090, 464,  1090, 2488,  1070, 484,  1068, 2490,  1068, 484,  1068, 486,  1068, 486,  1068,
 484,  1068, 486,  1068, 486,  1068, 486,  1066, 2470,  1088, 8880,  746, 6454,  744, 3606,  744, 6456,  742, 6458,  716, 6484,  714, 3636,  712, 6488,  710, 3640,  710, 6490,  706, 3644,  706, 3644, 
 706};

 uint16_t rawData_23[185] = {1108, 464,  1090, 486,  1068, 2490,  1068, 2490,  1068, 460,  1092, 462,  1090, 2490,  1070, 2488,  1070, 486,  1066, 486,
  1068, 2490,  1068, 484,  1068, 2490,  1068, 486,  1068, 486,  1068, 484,  1068, 2488,  1070, 484,  1070, 484,  1068, 486,  1068, 460,  1092, 486,  1070, 2488,  1068, 462,  1092, 484,  1068, 484,
    1070, 460,  1092, 2488,  1068, 462,  1092, 2490,  1068, 2488,  1068, 2490,  1070, 2488,  1068, 486,  1068, 484,  1068, 464,  1088, 486,  1072, 458,  1092, 462,  1090, 464,  1090, 2490,  1068, 464,  1090
, 484,  1068, 464,  1090, 486,  1068, 484,  1068, 486,  1068, 484,  1068, 484,  1068, 484,  1068, 484,  1068, 486,  1066, 488,  1066, 486,  1068, 486,  1066, 486,  1066, 488,  1064, 488,  1064, 490,  
1062, 492,  1062, 492,  1060, 2476,  1056, 2500,  1056, 500,  1052, 500,  1050, 502,  1052, 502,  1048, 504,  1048, 504,  1024, 2534,  1024, 528,  1024, 2534,  1022, 532,  1020, 532,  1020, 552,  1004
, 550,  1002, 552,  1000, 554,  1000, 554,  998, 556,  998, 8970,  656, 6544,  656, 3696,  654, 6546,  654, 6546,  652, 6548,  652, 3698,  650, 6550,  648, 3704,  642, 6580,  620, 3730,  620, 3730,  618};

uint16_t rawData_24_ON[185] = {1090, 486,  1070, 484,  1068, 2490,  1068, 2490,  1066, 486,  1068, 486,  1066, 2490,  1068, 2490,  1068, 464,  1090, 486,
  1066, 2490,  1070, 462,  1090, 2492,  1066, 468,  1086, 486,  1068, 486,  1068, 2490,  1068, 486,  1066, 464,  1090, 486,  1068, 486,  1066, 486,  1068, 2490,  1068, 486,  1068, 462,  1092, 462,
    1090, 464,  1090, 2490,  1068, 2490,  1068, 464,  1088, 462,  1092, 486,  1068, 2490,  1068, 484,  1068, 484,  1068, 462,  1090, 486,  1068, 486,  1068, 484,  1068, 462,  1092, 2490,  1070, 462,  1090, 
486,  1068, 464,  1090, 462,  1090, 462,  1092, 462,  1090, 486,  1068, 464,  1090, 486,  1068, 484,  1068, 484,  1068, 462,  1090, 486,  1068, 484,  1068, 484,  1068, 464,  1088, 462,  1092, 462, 
 1090, 462,  1092, 462,  1092, 2490,  1068, 2490,  1068, 486,  1068, 464,  1088, 464,  1090, 464,  1090, 484,  1068, 464,  1090, 2490,  1066, 486,  1068, 2490,  1068, 2490,  1068, 2490,  1068, 2490, 
  1068, 2490,  1068, 2490,  1068, 2490,  1068, 2468,  1088, 2470,  1090, 8878,  746, 6454,  746, 3604,  748, 6452,  746, 6454,  742, 6456,  742, 3610,  738, 6460,  712, 3638,  712, 6486,  710, 3640,  710, 
3640,  708};

uint16_t rawData_25[185] = {1088, 488,  1068, 486,  1066, 2490,  1068, 2490,  1068, 488,  1066, 462,  1090, 2490,  1068, 2490,  1066, 464,  1090, 466,
  1086, 2492,  1068, 486,  1068, 2490,  1068, 486,  1066, 464,  1090, 464,  1088, 2492,  1066, 488,  1066, 462,  1090, 462,  1092, 464,  1090, 486,  1066, 2492,  1066, 462,  1092, 486,  1066, 464, 
   1090, 464,  1088, 2492,  1066, 2490,  1068, 462,  1090, 488,  1066, 2492,  1066, 2492,  1066, 464,  1090, 466,  1088, 466,  1088, 464,  1088, 466,  1090, 486,  1068, 486,  1066, 2490,  1068, 484,  1070,
   484,  1068, 484,  1068, 486,  1066, 486,  1066, 488,  1066, 488,  1066, 488,  1064, 490,  1062, 490,  1062, 490,  1060, 494,  1062, 490,  1034, 520,  1032, 522,  1030, 522,  1030, 502,  1050, 502, 
    1050, 504,  1050, 504,  1046, 2512,  1024, 2534,  1024, 530,  1024, 530,  1024, 530,  1022, 532,  1022, 552,  1002, 552,  1000, 2558,  1000, 554,  1000, 2558,  998, 2560,  998, 2560,  998, 2560,  996, 
2562,  996, 2562,  996, 2564,  994, 2564,  994, 560,  992, 8978,  646, 6578,  620, 3730,  620, 6580,  618, 6582,  618, 6582,  618, 3732,  620, 6580,  618, 3732,  618, 6582,  618, 3732,  618, 3732,  618};

uint16_t rawData_26[185] = {1090, 486,  1068, 464,  1090, 2490,  1068, 2490,  1068, 462,  1092, 484,  1066, 2490,  1070, 2488,  1068, 486,  1068, 486,
  1068, 2490,  1068, 486,  1068, 2490,  1070, 460,  1092, 486,  1068, 486,  1068, 2488,  1070, 484,  1068, 464,  1090, 462,  1090, 486,  1066, 464,  1090, 2490,  1068, 464,  1090, 464,  1090, 464,
    1090, 462,  1090, 2492,  1068, 2490,  1068, 486,  1068, 2488,  1068, 464,  1088, 2490,  1068, 464,  1090, 464,  1090, 462,  1092, 464,  1088, 464,  1088, 464,  1088, 488,  1066, 2490,  1068, 484,
      1068, 484,  1068, 484,  1068, 486,  1068, 486,  1066, 486,  1068, 486,  1066, 488,  1064, 488,  1064, 488,  1064, 490,  1062, 490,  1062, 490,  1060, 492,  1034, 520,  1030, 522,  1030, 500, 
       1052, 502,  1050, 502,  1048, 504,  1048, 2510,  1048, 2510,  1022, 530,  1024, 530,  1024, 530,  1022, 532,  1020, 532,  1020, 552,  1000, 2558,  1000, 552,  1000, 2558,  998, 2558,  1000, 2560,  998, 2562,  996,
 2562,  996, 2562,  996, 2562,  994, 558,  994, 2566,  992, 8978,  644, 6556,  642, 3730,  620, 6580,  620, 6580,  618, 6582,  618, 3730,  620, 6580,  618, 3732,  618, 6580,  618, 3732,  618, 3732,  618};


uint16_t rawData_27[185] = {1072, 496,  1058, 494,  1060, 2520,  1038, 2520,  1034, 520,  1038, 514,  1066, 2492,  1048, 2510,  1042, 492,  1060, 514,
  1064, 2494,  1066, 466,  1086, 2494,  1066, 488,  1064, 490,  1064, 464,  1090, 2492,  1068, 462,  1090, 488,  1066, 464,  1088, 464,  1088, 466,  1086, 2494,  1068, 462,  1090, 464,  1090, 462,
    1090, 488,  1066, 2492,  1068, 2490,  1068, 466,  1088, 2492,  1066, 2492,  1066, 2492,  1066, 466,  1088, 2490,  1068, 2490,  1066, 2492,  1066, 488,  1066, 2490,  1068, 2490,  1068, 486,  1068, 486,  
1068, 2492,  1066, 2468,  1088, 2470,  1088, 488,  1064, 2470,  1088, 2470,  1086, 490,  1062, 492,  1060, 494,  1034, 520,  1034, 520,  1032, 502,  1050, 502,  1050, 2506,  1050, 2508,  1048, 506,
  1048, 2510,  1024, 2534,  1024, 528,  1024, 2534,  1022, 532,  1022, 2538,  1020, 552,  1000, 554,  998, 554,  1000, 554,  1000, 554,  1000, 2558,  1000, 554,  998, 2560,  998, 556,  996, 2564,  996,
   558,  996, 2564,  992, 560,  994, 2566,  990, 2570,  986, 590,  962, 9008,  620, 6578,  620, 3730,  618, 6582,  618, 6582,  618, 6582,  618, 3732,  618, 6582,  618, 3734,  616, 6608,  592, 3758,  580, 
3770,  570};

uint16_t rawData_28[185] = {1074, 498,  1054, 520,  1034, 2524,  1034, 2524,  1036, 518,  1034, 496,  1058, 2522,  1038, 2522,  1036, 518,  1036, 494,
  1058, 2522,  1038, 494,  1058, 2522,  1034, 518,  1036, 496,  1060, 516,  1036, 2520,  1066, 488,  1036, 494,  1062, 494,  1058, 496,  1056, 518,  1036, 2522,  1040, 492,  1060, 516,  1038, 492,
    1060, 516,  1038, 2520,  1034, 2524,  1036, 2522,  1038, 492,  1060, 494,  1056, 2524,  1034, 498,  1058, 2522,  1036, 2522,  1066, 2494,  1034, 496,  1058, 2524,  1038, 2520,  1038, 512,  1038, 518,
      1036, 2522,  1038, 2520,  1036, 2522,  1040, 514,  1038, 2520,  1036, 2524,  1038, 494,  1060, 516,  1040, 492,  1056, 520,  1034, 496,  1060, 494,  1060, 492,  1058, 2524,  1034, 2526,  1036, 494, 
       1058, 2522,  1034, 2524,  1036, 494,  1060, 2522,  1040, 490,  1060, 2522,  1038, 516,  1038, 514,  1064, 490,  1064, 490,  1066, 486,  1066, 2492,  1068, 486,  1066, 2492,  1064, 488,  1064, 2470, 
        1086, 490,  1064, 2470,  1086, 490,  1060, 2476,  1058, 518,  1034, 2500,  1054, 8914,  712, 6488,  710, 3640,  710, 6490,  708, 6492,  706, 6494,  682, 3668,  682, 6518,  680, 3670,  680, 6520,
          680, 3670,  678, 3674,  678};


          uint16_t rawData_29[185] = {1072, 520,  1008, 546,  1032, 2526,  1036, 2522,  1038, 516,  1034, 518,  1034, 2524,  1034, 2524,  1040, 494,  1054, 520,
  1034, 2524,  1036, 518,  1034, 2522,  1036, 518,  1036, 496,  1056, 494,  1058, 2524,  1036, 496,  1056, 520,  1034, 494,  1058, 498,  1058, 496,  1058, 2522,  1036, 494,  1060, 518,  1034, 498, 
   1056, 498,  1054, 2522,  1038, 2522,  1038, 2520,  1034, 520,  1038, 2518,  1040, 2518,  1038, 494,  1060, 2520,  1040, 2518,  1046, 2512,  1038, 492,  1060, 2522,  1040, 2518,  1064, 468,  1060, 492,  
1062, 2520,  1040, 2518,  1064, 2494,  1038, 494,  1086, 2494,  1040, 2518,  1040, 490,  1064, 492,  1062, 514,  1064, 464,  1088, 468,  1062, 492,  1086, 490,  1062, 2494,  1066, 2492,  1064, 490,
  1064, 2494,  1066, 2492,  1066, 488,  1066, 2492,  1068, 486,  1068, 2492,  1066, 486,  1066, 486,  1068, 486,  1066, 486,  1066, 488,  1064, 2470,  1088, 488,  1062, 2472,  1084, 492,  1060, 2476,
    1058, 518,  1032, 2504,  1054, 500,  1050, 2506,  1052, 502,  1048, 504,  1048, 8920,  680, 6516,  682, 3670,  680, 6518,  680, 6520,  678, 6520,  678, 3690,  658, 6540,  658, 3692,  658, 6542,
      656, 3694,  656, 3694,  656};

uint16_t rawData_30[185] = {1058, 496,  1056, 494,  1058, 2524,  1036, 2522,  1036, 518,  1034, 520,  1038, 2520,  1038, 2520,  1034, 494,  1060, 496,
  1058, 2520,  1038, 516,  1038, 2520,  1038, 494,  1058, 518,  1036, 494,  1058, 2522,  1038, 516,  1034, 496,  1058, 496,  1056, 498,  1058, 516,  1036, 2520,  1040, 514,  1038, 516,  1038, 516,
    1036, 516,  1038, 2520,  1040, 2518,  1036, 2522,  1038, 2520,  1040, 514,  1038, 2518,  1038, 516,  1036, 2522,  1040, 2518,  1040, 2518,  1066, 468,  1084, 2494,  1062, 2494,  1068, 464,  1088, 464,  
1090, 2492,  1066, 2492,  1066, 2492,  1066, 488,  1064, 2492,  1068, 2492,  1066, 466,  1086, 466,  1088, 466,  1088, 466,  1088, 466,  1086, 490,  1064, 464,  1088, 2494,  1066, 2492,  1068, 486,
  1068, 2490,  1068, 2490,  1068, 486,  1068, 2468,  1088, 488,  1066, 2468,  1088, 488,  1066, 488,  1064, 490,  1064, 490,  1060, 492,  1060, 2476,  1058, 518,  1034, 2500,  1052, 524,  1030, 2504,
    1052, 502,  1050, 2506,  1050, 504,  1026, 528,  1024, 2534,  1024, 2536,  1022, 8946,  680, 6520,  678, 3672,  680, 6538,  658, 6542,  658, 6540,  658, 3692,  658, 6542,  656, 3694,  656, 6544,  654,
     3696,  654, 3696,  652};

uint16_t rawData_24_OFF[185] = {1028, 552,  1002, 552,  1004, 2552,  1008, 2552,  1006, 524,  1028, 522,  1030, 2554,  1004, 2554,  1004, 550,  1002, 526, 
 1050, 2534,  1002, 526,  1028, 2532,  1026, 522,  1030, 528,  1024, 526,  1028, 524,  1030, 524,  1028, 524,  1028, 526,  1028, 528,  1050, 500,  1028, 2554,  1004, 524,  1026, 526,  1028, 524,  1028
, 524,  1030, 2532,  1026, 2534,  1026, 526,  1028, 524,  1028, 524,  1052, 2506,  1028, 524,  1028, 2530,  1026, 2534,  1024, 2532,  1026, 526,  1028, 2530,  1028, 2532,  1024, 528,  1026, 526,  1026
, 2534,  1026, 2532,  1026, 2532,  1026, 526,  1028, 2532,  1024, 2536,  1024, 526,  1028, 526,  1026, 528,  1026, 526,  1028, 526,  1028, 526,  1026, 528,  1028, 2532,  1024, 2534,  1024, 528,  1026,
 2534,  1024, 2534,  1024, 528,  1028, 2532,  1050, 502,  1028, 2532,  1050, 504,  1026, 526,  1026, 528,  1026, 528,  1026, 528,  1024, 2534,  1024, 528,  1024, 2534,  1024, 2534,  1022, 2534,  1024,
 530,  1022, 2536,  1022, 2536,  1022, 532,  1020, 548,  1006, 2558,  1000, 8970,  656, 6542,  658, 3692,  656, 6542,  656, 6544,  656, 6544,  654, 3696,  654, 3698,  652, 6546,  650, 6550,  650, 3700
,  648, 6552,  644};

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
   PRINT SAVED IR DATA
//    ========================================================= */
// void printSavedIRData(uint8_t keyIndex)
// {
//     if (keyIndex >= TOTAL_IR_KEYS)
//     {
//         Serial.println("Invalid Key Index");
//         return;
//     }

//     Serial.println();
//     Serial.printf("========== KEY %d ==========\n", keyIndex);
//     Serial.printf("Length : %u\n", irRawLength);
//     Serial.print("Raw Data : ");

//     for (uint16_t i = 0; i < irRawLength; i++)
//     {
//         // Serial.print(irRawData[i]);

//         if (i < (irRawLength - 1))
//             Serial.print(", ");
//     }

//     Serial.println();
//     Serial.println("============================");
// }

/* =========================================================
   CLEAR EEPROM
   ========================================================= */
// void clearEEPROM()
// {
//     for (uint8_t key = 0; key < TOTAL_IR_KEYS; key++)
//     {
//         int base = getEEPROMBaseAddress(key);
//         EEPROM.write(base, 0x00);
//         EEPROM.write(base + 1, 0x00);
//     }

//     EEPROM.commit();
//     irRawLength = 0;
// }

// void saveIRCodeToEEPROM(uint8_t keyIndex) {

//   int base = getEEPROMBaseAddress(keyIndex);
//   uint16_t len = irRawLength;

//   EEPROM.write(base, len & 0xFF);
//   EEPROM.write(base + 1, len >> 8);

//   for (uint16_t i = 0; i < len; i++) {
//     EEPROM.write(base + 2 + i * 2, irRawData[i] & 0xFF);
//     EEPROM.write(base + 3 + i * 2, irRawData[i] >> 8);
//   }

//   EEPROM.commit();
// }

// bool loadIRCodeFromEEPROM(uint8_t keyIndex) {
//   int base = getEEPROMBaseAddress(keyIndex);
//   uint16_t len = EEPROM.read(base) | (EEPROM.read(base + 1) << 8);

//   if (len == 0 || len > IR_RAW_BUFFER_SIZE) {
//     return false;
//   }

//   irRawLength = len;

//   for (uint16_t i = 0; i < len; i++) {
//     irRawData[i] =
//       EEPROM.read(base + 2 + i * 2) |
//       (EEPROM.read(base + 3 + i * 2) << 8);
//   }
//   return true;
// }

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


    if(temp == 16) {
      irSender.sendRaw(rawData_16, irRawLength, IR_CARRIER_FREQ);
    }else if(temp == 17) {
      irSender.sendRaw(rawData_17, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 18) {
      irSender.sendRaw(rawData_18, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 19) {
      irSender.sendRaw(rawData_19, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 20) {
      irSender.sendRaw(rawData_20, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 21) {
      irSender.sendRaw(rawData_21, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 22) {
      irSender.sendRaw(rawData_22, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 23) {
      irSender.sendRaw(rawData_23, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 24) {
      irSender.sendRaw(rawData_24_ON, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 25) {
      irSender.sendRaw(rawData_25, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 26) {
      irSender.sendRaw(rawData_26, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 27) {
      irSender.sendRaw(rawData_27, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 28) {
      irSender.sendRaw(rawData_28, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 29) {
      irSender.sendRaw(rawData_29, irRawLength, IR_CARRIER_FREQ);
    } else if(temp == 30) {
      irSender.sendRaw(rawData_30, irRawLength, IR_CARRIER_FREQ);
    }
}

/* =========================================================
   AC CONTROL
   ========================================================= */
void updateACState(int brightness) {

  static int lastTemp = -1;

  if (brightness < 15) {
    if (isAcOn) {
      Serial.println("AC OFF");
      irSender.sendRaw(rawData_24_OFF, irRawLength, IR_CARRIER_FREQ);
    //   sendIRCommand(AC_OFF_LOCATION);
      isAcOn = false;
    }
    return;
  }

  if (!isAcOn) {
    Serial.println("AC ON");
    irSender.sendRaw(rawData_24_ON, irRawLength, IR_CARRIER_FREQ);
    // sendIRCommand(AC_ON_LOCATION);
    delay(2000);
    isAcOn = true;
  }

  int targetTemp = mapBrightnessToTemperature(brightness);
  Serial.printf("Brightness: %d → Temp: %d\n", brightness, targetTemp);


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
// bool learnIRCommand(uint8_t keyIndex, unsigned long sessionStart)
// {
//     unsigned long start = millis();
//     while (millis() - start < IR_LEARN_TIMEOUT_MS)
//     {
//         if (millis() - sessionStart >= LEARNING_SESSION_TIMEOUT_MS)
//         {
//             Serial.println("Learning session timeout");
//             return false;
//         }
//         digitalWrite(PIN_STATUS_LED, LOW);
//         delay(120);
//         digitalWrite(PIN_STATUS_LED, HIGH);
//         delay(120);

//         if (irReceiver.decode(&irResults))
//         {
//             uint16_t len = irResults.rawlen - 1;
//             /* Reject invalid/short frames */
//             if (len < MIN_IR_FRAME_LENGTH)
//             {
//                 Serial.printf("Invalid IR Frame (Length=%u). Waiting for valid frame...\n", len);
//                 irReceiver.resume();
//                 continue;
//             }

//             /* Clear EEPROM only once after first valid frame */
//             if (!eepromClearedThisSession)
//             {
//                 clearEEPROM();
//                 eepromClearedThisSession = true;
//             }

//             if (len > IR_RAW_BUFFER_SIZE)
//                 len = IR_RAW_BUFFER_SIZE;

//             irRawLength = len;

//             for (uint16_t i = 1; i <= len; i++)
//             {
//                 irRawData[i - 1] = irResults.rawbuf[i] * kRawTick;
//             }

//             saveIRCodeToEEPROM(keyIndex);
//             irReceiver.resume();
//             printSavedIRData(keyIndex);
//             Serial.printf("Saved Key-%d (Length=%u)\n", keyIndex, len);
//             delay(500);
//             return true;
//         }
//     }

//     return false;
// }


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
        sendACTemperature(24);
        // sendIRCommand(AC_ON_LOCATION);
        delay(200);
        isAcOn = true;
    }

    int targetTemp = uartTargetTemp;
    if (targetTemp < 16 || targetTemp > 30)
    {
        return;
        //targetTemp = defaultAcTemp;
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
            // sendIRCommand(AC_ON_LOCATION);
            sendACTemperature(24);
            isAcOn = true;

            extUart.println("ON_ACK");
            Serial.println("UART: AC ON");
        
        }else if (cmd == "OFF"){
            // sendIRCommand(AC_OFF_LOCATION);
            irSender.sendRaw(rawData_24_OFF, irRawLength, IR_CARRIER_FREQ);
            isAcOn = false;

            extUart.println("OFF_ACK");
            Serial.println("UART: AC OFF");
        
        }else if (cmd.startsWith("TEMP:")){
            int temp = cmd.substring(5).toInt();
            if (temp >= 16 && temp <= 30)
            {
                sendACTemperature(temp);
                uartTargetTemp = temp;
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


// void startIRLearningSequence()
// {
//     Serial.println("Start Learning Mode");
//     isLearningMode = true;
//     eepromClearedThisSession = false;
//     unsigned long sessionStart = millis();
//     for (uint8_t i = 0; i < TOTAL_IR_KEYS; i++) {

//         if (millis() - sessionStart >= LEARNING_SESSION_TIMEOUT_MS) {
//             Serial.println("Learning mode expired");
//             break;
//         }

//         if (i == AC_OFF_LOCATION)
//             Serial.println("Learn AC OFF");
//         else if (i == AC_ON_LOCATION)
//             Serial.println("Learn AC ON");
//         else
//             Serial.printf("Learn TEMP %d\n", 14 + i);

//         if (!learnIRCommand(i, sessionStart)) {
//             break;
//         }

//         delay(1000);
//     }

//     Serial.println("Learning mode exited");
//     isLearningMode = false;
// }

/* =========================================================
   SETUP
   ========================================================= */
void setup() {

  Serial.begin(115200);
  extUart.begin(115200);
//   EEPROM.begin(EEPROM_TOTAL_SIZE);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_PWM_INPUT, INPUT);

  digitalWrite(PIN_STATUS_LED, HIGH);

  irReceiver.enableIRIn();
  irSender.begin();
//   for (uint8_t i = 0; i < TOTAL_IR_KEYS; i++) {
//     loadIRCodeFromEEPROM(i);
//   }

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
