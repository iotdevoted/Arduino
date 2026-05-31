#include "Config.h"
volatile bool Timer_Flag = false;
// ---------------------------------------------------------------------Timer_Section------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------------------------
hw_timer_t *timer = NULL; 
void IRAM_ATTR onTimer()
{
    Timer_Flag = true;
}


//Setup callbacks onConnect and onDisconnect for BLE Connection
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };
    
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        pServer->getAdvertising()->start();
    };
};


class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {

        String rxValueRaw = pCharacteristic->getValue();
        String rxValue = String(rxValueRaw.c_str());

        if (rxValue.length() == 0) return;

        int len = rxValue.length();
        if (len < 3) return;  
        Serial.print("BLE RX: ");
        Serial.println(rxValue);


        char SOM = rxValue[0];

        // ----- MOTOR ROTATION COMMAND -----
        if (SOM == BLE_SOM_Ps || SOM == BLE_SOM_Pc) {
            if (len < 12) return;  // ensure safe access
            bool singleDigitMotor = (rxValue[9] == '#');
            if (singleDigitMotor) {
                Motor_no = rxValue[8] - '0';
                Rotate_count = rxValue[10] - '0';
            } else {
                Motor_no = (rxValue[8] - '0') * 10 + (rxValue[9] - '0');
                Rotate_count = rxValue[11] - '0';
            }
            Motor_RotateFlag = true;
            return;
        }

        // ----- MOTOR RESET COMMAND -----
        if ((SOM == BLE_SOM_Ms || SOM == BLE_SOM_Mc) && len >= 6) {
            if (rxValue[1] == '1' && rxValue[5] == '1') {
                Motor_ResetFlag = true;
            }
            return;
        }
    }
};

void WritePort1AndPort4(uint8_t value)
{
    // Existing PortExp_1
    Wire.beginTransmission(PortExp_1);
    Wire.write(value);
    Wire.endTransmission();

    // Mirror to PortExp_4
    Wire.beginTransmission(PortExp_4);
    Wire.write(value);
    Wire.endTransmission();

    Port4Active = true;
    Port4StartTime = millis();
}


// Setup Port_A & Port_B 's values
void Motor_Rotate(unsigned int Number_of_Motor) {
    PortA_value = 1;
    PortB_value = 0;
    unsigned int shift_amount = Number_of_Motor / 10;
    unsigned int adjusted_motor = Number_of_Motor % 10;
    PortA_value <<= shift_amount;

    if (adjusted_motor < 3)
        PortA_value += (1 << (5 + adjusted_motor));
    else 
        PortB_value = (1 << (adjusted_motor - 3));
}


// Validating he rotation of motor by checking Pin1 (Low, High, Low)
void Rotate_Count(){
    timerRestart(timer);
    timerStart(timer);

        while(digitalRead(buttonPin1) == LOW)//EXTRA 02102024
        {
            Serial.println(digitalRead(buttonPin1)); //Extra 16-05-2024
            Serial.println("MP1");
            if(Timer_Flag)
                break;
        }
        delay(500);
        while(digitalRead(buttonPin1) == HIGH) //EXTRA 02102024
        {
            Serial.println(digitalRead(buttonPin1));  //Extra 16-05-2024
            Serial.println("MP2");
            if(Timer_Flag)
                break;

            Serial.println(digitalRead(buttonPin1));  //Extra 16-05-2024
            Serial.println("Motor Stop");
        }

    Serial.println(digitalRead(buttonPin1)); //Extra        
    delay(100);
    timerStop(timer);

    Timer_Flag = false;
    Wire.beginTransmission(PortExp_3);
    Wire.write((255));          //Stop All Motors
    Wire.endTransmission();
    Wire.beginTransmission(PortExp_2);
    Wire.write((255));          //Stop All Motors
    Wire.endTransmission();
    WritePort1AndPort4(255);
    Wire.endTransmission();
}


void BLE_Init(void){
    // Create the BLE Device
    BLEDevice::init(bleServerName);
    BLEDevice::setMTU(30);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  

    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID,
                    BLECharacteristic::PROPERTY_READ |
                    BLECharacteristic::PROPERTY_WRITE |
                    BLECharacteristic::PROPERTY_NOTIFY |
                    BLECharacteristic::PROPERTY_INDICATE);
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCallbacks());                                 
    pService->start();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE STARTED V1.1");
}


void setup() {
    Serial2.begin(9600);
    Serial.begin(115200);
    
    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(Door1, OUTPUT);
    pinMode(Door2, OUTPUT);
    digitalWrite(Door1,HIGH);
    digitalWrite(Door2,HIGH);
    
    BLE_Init();
    card_no[0] = 'R';
    card_no[1] = ':';
    card_no[14] = '\0';
    
    complete_feedback[0] = 'c';
    complete_feedback[1] = 'o';
    complete_feedback[2] = 'm';
    complete_feedback[3] = 'p';
    complete_feedback[4] = 'l';
    complete_feedback[5] = 'e';
    complete_feedback[6] = 't';
    complete_feedback[7] = 'e';
    complete_feedback[8] = 'd';
    complete_feedback[9] = '#';
    
    Wire.begin(); // wake up I2C
    Wire.beginTransmission(PortExp_3);
    Wire.write(255);
    Wire.endTransmission();
    delay(500);
    Wire.beginTransmission(PortExp_2);
    Wire.write(255);
    Wire.endTransmission();
    delay(500);
    WritePort1AndPort4(255);
    delay(10);

    timer = timerBegin(1000);       //the timer ticks at 1000 Hz
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, 15000000ULL, true, 0);        //15 sec time
    timerStop(timer);

}

void loop() {
    delay(10);
    if (Port4Active && (millis() - Port4StartTime >= PORT4_ON_TIME))
    {
        Wire.beginTransmission(PortExp_4);
        Wire.write(0xFF);
        Wire.endTransmission();

        Port4Active = false;

        Serial.println("PortExp_4 Auto OFF");
    }

    if (deviceConnected){
        if(Serial2.available()){
            count = 2;
            while(Serial2.available() && count <= 13){
                card_no[count] = Serial2.read();
                count++;
                delay(2);
            }
          
            Serial.println(card_no);
            if( strcmp(card_no, Maint_Card1) == 0 || 
                strcmp(card_no, Maint_Card2) == 0 || 
                strcmp(card_no, Maint_Card3) == 0 || 
                strcmp(card_no, Maint_Card4) == 0 || 
                strcmp(card_no, Maint_Card5) == 0  )
            {
                digitalWrite(Door1,LOW);
                digitalWrite(Door2,LOW);
                delay(5000);
                digitalWrite(Door1,HIGH);
                digitalWrite(Door2,HIGH);
            }
          
            pCharacteristic->setValue(card_no);
            pCharacteristic->notify();
            Serial.print("BLE TX (Card): ");
            Serial.println(card_no);
        }
    }

    if(Motor_RotateFlag == true){
        Serial.println(Motor_no);
        if( (Motor_no>90) && (Motor_no<=98) ){
            Wire.beginTransmission(PortExp_3);
            switch(Motor_no)
            {
                case 91:Wire.write(0xFE);break;
                case 92:Wire.write(0xFD);break;
                case 93:Wire.write(0xFB);break;
                case 94:Wire.write(0xF7);break;
                case 95:Wire.write(0xEF);break;
                case 96:Wire.write(0xDF);break;
                case 97:Wire.write(0xBF);break;
                //case 98:Wire.write(0x7F);break; 
                default:Wire.write(0XFF);break; 
            }
            Wire.endTransmission();
            delay(1200);      // here is the delay for change // Conveyour belt rotation count.
            Wire.beginTransmission(PortExp_3);
            Wire.write(0XFF);
            Wire.endTransmission();
            delay(10);
        
        }else{
            Motor_Rotate(Motor_no);
            Serial.println(PortA_value);
            Serial.println(PortB_value);
            Serial.println(Rotate_count);
            Wire.beginTransmission(PortExp_2);
            Wire.write((255-PortA_value));    // PORT A
            Wire.endTransmission();
            delay(1000); // for debounce
            WritePort1AndPort4(255 - PortB_value);

            for(int j = 0; j < Rotate_count; j++)
                Rotate_Count();
        
            if(Motor_no < 10)
            {
                complete_feedback[10] = '0' + Motor_no;
                complete_feedback[11] = '\0';
            }
            else
            {
                complete_feedback[10] = '0' + (Motor_no / 10);
                complete_feedback[11] = '0' + (Motor_no % 10);
                complete_feedback[12] = '\0';
            }
        }
        delay(500);  //     befor Order complete_feedback delay
        pCharacteristic->setValue(complete_feedback);
        pCharacteristic->notify();
        Motor_RotateFlag = false;

        Serial.print("BLE TX (Feedback): ");
        Serial.println(complete_feedback);
    }
}