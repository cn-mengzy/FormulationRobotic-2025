/*endstop common→GND  NO→Endstop
based on arduino mega 2560 with ramps1.4*/
//version
String versionN2 = "Node04v2024-08-30";
// 1.libraries
#include <RF24.h>
#include <EEPROM.h>

//2.Define
//motorX
#define motor_x_dir 55
#define motor_x_step 54
#define switch_x_endstop_max 2
#define switch_x_endstop_min 3
#define enable_x 38
//motorY
#define motor_y_dir 61
#define motor_y_step 60
#define switch_y_endstop_max 15
#define switch_y_endstop_min 14
#define enable_y 56
//motorZ
#define motor_z_dir 48
#define motor_z_step 46
#define switch_z_endstop_max 19
#define switch_z_endstop_min 18
#define enable_z 62
//motorE
#define motor_e_dir 28
#define motor_e_step 26
#define switch_e_endstop_max 4
#define switch_e_endstop_min 5
#define enable_e 24
//switch
#define switch_mode 0 //swtich mode
//rf
#define rf_MISO 50
#define rf_MOSI 51
#define rf_SCK 52
#define rf_CE 49
#define rf_CSN 47
// button-Move
#define button_moveMax_x 6  // X轴正向按钮
#define button_moveMin_x 8  // X轴负向按钮
#define button_moveMax_y 9  // Y轴正向按钮
#define button_moveMin_y 10  // Y轴负向按钮
#define button_moveMax_z 11 // Z轴正向按钮
#define button_moveMin_z 16 // Z轴负向按钮
#define button_moveMax_e 17 // E轴正向按钮
#define button_moveMin_e 23 // E轴负向按钮
// button-Reset
#define button_reset_x 29  // X轴自动复位按钮
#define button_reset_y 31  // Y轴自动复位按钮
#define button_reset_z 32  // Z轴自动复位按钮
#define button_reset_e 33  // E轴自动复位按钮
#define button_reset_all 35  // 全自动复位按钮
// Led
#define led_1_buttonMode 25  // LED1，当手动模式（isManualMode为true）时亮
#define led_2_rfMode 27  // LED2，当RF模式（isManualMode为false）时亮
// 2. Node ID
const int nodeId = 4;
// 3. Variables
long speed_x, speed_y, speed_z, speed_e;
long x,y,z,e,step_x,step_y,step_z,step_e;
unsigned long startTime,endTime;
// 4. RF object create
RF24 radio(rf_CE, rf_CSN); // CE, CSN
const uint64_t rxAddress1 = 0xE8E8F0F0D1LL; // RX_1
const uint64_t rxAddress2 = 0xE8E8F0F0D1LL; // RX_2
const uint64_t txAddress1 = 0xE8E8F0F0A1LL; // TX_to_Node1
const uint8_t channel_A1_90 = 90; // RF channel
const int MAX_MESSAGE_LENGTH = 32;
// 5.Custom function/def/void
long matchNumber(String command, String keyword) {
    int c_start = command.indexOf(keyword);
    if (c_start != -1) {
        //  
        c_start += keyword.length();  // 移动到关键字之后的数字的开始位置
        int c_end = c_start;            // 初始化结束位置
        // 找到数字部分的结束位置
        while (c_end < command.length() && isDigit(command[c_end])) {
            c_end++;
        }
        String numStr = command.substring(c_start, c_end);
        // 使用strtol从字符串中解析长整型
        char* endPtr;
        long num = strtol(numStr.c_str(), &endPtr, 10);
        if (*endPtr == '\0') {  // 确保整个字符串都是数字
            return num;
        }
    }
    return -1;  // 如果解析失败或找不到关键字，则返回-1
}
void reply(char message_r[]){
    radio.stopListening(); // 停止接收模式，进入发送模式
    radio.openWritingPipe(txAddress1); // 设置发送地址为 txAddress1
    radio.write(message_r, strlen(message_r) + 1); // 发送消息
    radio.startListening(); // 进入接收模式
    radio.flush_rx();
    radio.flush_tx();
}
void runMotor (int PIN_Step, int PIN_Dir, int ENDSTOP_Max,int ENDSTOP_Min, long SPEED, long STEP, bool direction) {
        if (direction)
        {
            digitalWrite(PIN_Dir, HIGH);
        }
        else if (!direction)
        {
            digitalWrite(PIN_Dir, LOW);
        }
        if(STEP>0){
            long STEP_Count = 1;
            while( STEP >= STEP_Count){
                //松开是High 按下是Low default is high,press is low
                if(digitalRead(ENDSTOP_Min) == HIGH && digitalRead(ENDSTOP_Max) == HIGH){
                    digitalWrite(PIN_Step, HIGH);
                    delay(SPEED);
                    digitalWrite(PIN_Step, LOW);
                    delay(SPEED);
                    STEP_Count ++;
                }
                else if (digitalRead(ENDSTOP_Min) == LOW)
                {
                    goToMax(PIN_Step, PIN_Dir, ENDSTOP_Max, ENDSTOP_Min, SPEED);
                }
            }
        }
}
void goToMax (int PIN_Step, int PIN_Dir, int ENDSTOP_Max,int ENDSTOP_Min, long SPEED){
    digitalWrite(PIN_Dir, HIGH);//counter clockwise
    while (digitalRead(ENDSTOP_Max) == HIGH)
    {
        digitalWrite(PIN_Step, HIGH);
        delay(SPEED);
        digitalWrite(PIN_Step, LOW);
        delay(SPEED);
    }
    while (digitalRead(ENDSTOP_Max) == LOW)
    {
        digitalWrite(PIN_Dir, LOW);
        int i;
        for(i=1; i<=5; i++){
            digitalWrite(PIN_Step, HIGH);
            delay(SPEED);
            digitalWrite(PIN_Step, LOW);
            delay(SPEED);
            }
    }
}
//6. Main ↓ setup+loop
void setup() {
    pinMode(motor_x_dir,OUTPUT);
    pinMode(motor_x_step,OUTPUT);
    pinMode(enable_x, OUTPUT);
    digitalWrite(enable_x, LOW);
    pinMode(button_moveMax_x, OUTPUT);
    pinMode(button_moveMin_x, OUTPUT);
    pinMode(button_reset_x, OUTPUT);
    pinMode(switch_x_endstop_max, INPUT_PULLUP);
    pinMode(switch_x_endstop_min, INPUT_PULLUP);
    pinMode(motor_y_dir,OUTPUT);
    pinMode(motor_y_step,OUTPUT);
    pinMode(enable_y, OUTPUT);
    digitalWrite(enable_y, LOW);
    pinMode(button_moveMax_y, INPUT_PULLUP);
    pinMode(button_moveMin_y, INPUT_PULLUP);
    pinMode(button_reset_y, INPUT_PULLUP);
    pinMode(switch_y_endstop_max, INPUT_PULLUP);
    pinMode(switch_y_endstop_min, INPUT_PULLUP);
    pinMode(motor_z_dir,OUTPUT);
    pinMode(motor_z_step,OUTPUT);
    pinMode(enable_z, OUTPUT);
    digitalWrite(enable_z, LOW);
    pinMode(button_moveMax_z, INPUT_PULLUP);
    pinMode(button_moveMin_z, INPUT_PULLUP);
    pinMode(button_reset_z, INPUT_PULLUP);
    pinMode(switch_z_endstop_max, INPUT_PULLUP);
    pinMode(switch_z_endstop_min, INPUT_PULLUP);
    pinMode(motor_e_dir,OUTPUT);
    pinMode(motor_e_step,OUTPUT);
    pinMode(enable_e, OUTPUT);
    digitalWrite(enable_e, LOW);
    pinMode(button_moveMax_e, INPUT_PULLUP);
    pinMode(button_moveMin_e, INPUT_PULLUP);
    pinMode(button_reset_e, INPUT_PULLUP);
    pinMode(switch_e_endstop_max, INPUT_PULLUP);
    pinMode(switch_e_endstop_min, INPUT_PULLUP);
    pinMode(button_reset_x, INPUT_PULLUP);
    pinMode(button_reset_y, INPUT_PULLUP);
    pinMode(button_reset_z, INPUT_PULLUP);
    pinMode(button_reset_all, INPUT_PULLUP);

    pinMode(switch_mode, INPUT_PULLUP);

    EEPROM.get(0, speed_x);
    EEPROM.get(sizeof(long), speed_y);
    EEPROM.get(2*sizeof(long), speed_z);
    EEPROM.get(3*sizeof(long), speed_e);
    // Open Serial
    Serial.begin(115200);
    //Turn on RF
    radio.begin();
    radio.setChannel(channel_A1_90);
    radio.openWritingPipe(txAddress1); // setting sending address
    radio.openReadingPipe(1, rxAddress1); // setting receive address
    radio.setPALevel(RF24_PA_LOW);
    radio.setRetries(15, 15);
    radio.startListening();
    Serial.println("ok");
}
void loop() {
    // 从串口接收数据并添加到队列
    if (Serial.available()) {
        char message[MAX_MESSAGE_LENGTH + 1];
        size_t bytesRead = Serial.readBytesUntil('\n', message, MAX_MESSAGE_LENGTH);
        message[bytesRead] = '\0'; // Add null-terminator
        int targetNodeId = matchNumber(message, "$");
        reply(message);
    }
    if (radio.available()) {
        char message[MAX_MESSAGE_LENGTH + 1];
        radio.read(message, MAX_MESSAGE_LENGTH + 1);
        message[MAX_MESSAGE_LENGTH] = '\0';
        int targetNodeId = matchNumber(message, "$");
        if(targetNodeId == nodeId){
            int c = matchNumber(message,"c");
//            Serial.print("c = ");
//            Serial.println(c);
            x= matchNumber(message, "x");
            y = matchNumber(message, "y");
            z = matchNumber(message, "z");
            e = matchNumber(message, "e");
//            Serial.print("x = ");
//            Serial.println(x);
            switch (c) {
                case 0:
                    reply("ok");
                    //Serial.println("case 0 RE");
                    char message[32]; // 创建一个足够大的字符数组来保存消息
                    strcpy(message, "Robot3 Node4 for PUMP");
                    reply(message); // 发送消息
                    delayMicroseconds(10);
                    strcpy(message, "c1: Reply ok");
                    reply(message); // 发送消息
                    delayMicroseconds(10);
                    //message4 = "c2: Run motor, step";
                    strcpy(message, "c2: Run motor(step)");
                    reply(message);
                    delayMicroseconds(10);
                    //message4 = "c3: Set motor speed and Write into EEPROM";
                    strcpy(message, "c3: Set motor speed");
                    reply(message);
                    delayMicroseconds(10);
                    //message4 = "c4: Ask motor speed";
                    strcpy(message, "c4: Ask motor speed");
                    reply(message);
                    delayMicroseconds(10);
                    strcpy(message, "c5: Hand Control X");
                    reply(message);
                    delayMicroseconds(10);
                    strcpy(message, "c6: Hand Control Y");
                    reply(message);
                    delayMicroseconds(10);
                    strcpy(message, "c7: Hand Control Z");
                    reply(message);
                    delayMicroseconds(10);
                    strcpy(message, "c8: For TESTING Only");
                    reply(message);
                    delayMicroseconds(10);
                    strcpy(message, "c9: coding version");
                    reply(message);
                    //delayMicroseconds(10);
                    reply("!");
                    radio.flush_rx();
                    radio.flush_tx();
                    break;

                case 1://reply ok
                    reply("ok");
                    delayMicroseconds(10);
                    reply("!");
                    break;

                case 2://runMotor receive step
                    reply("ok");
                    
                    delayMicroseconds(10);
                    if(x > 0){    
                       runMotor (motor_x_step, motor_x_dir, switch_x_endstop_max,switch_x_endstop_min, speed_x,x,0);
                    }
                    if(y > 0){
                      runMotor (motor_y_step, motor_y_dir, switch_y_endstop_max,switch_y_endstop_min, speed_y,y,0);
                    }
                    if(z > 0){
                       runMotor (motor_z_step, motor_z_dir, switch_z_endstop_max,switch_z_endstop_min, speed_z,z,0);
                    }
                    if(e > 0){
                       runMotor (motor_e_step, motor_e_dir, switch_e_endstop_max,switch_e_endstop_min, speed_e,e,0);
                    }
                    delayMicroseconds(10);
                    reply("c2!");
                    break;

                case 3:
                    reply("ok");
                    //x = matchNumber(message, "x");
//                    y = matchNumber(message, "y");
//                    z = matchNumber(message, "z");
//                    e = matchNumber(message, "e");
                    Serial.print("Parsed x: "); Serial.println(x);
                    Serial.print("Parsed y: "); Serial.println(y);
                    Serial.print("Parsed z: "); Serial.println(z);
                    Serial.print("Parsed e: "); Serial.println(e);
                    if (x > 0){
                    //   motorspeed_x=x;
                    //   Serial.println(motorspeed_x);
                      speed_x = x;
                      EEPROM.put(0, speed_x);
                    }

                    if(y > 0){
                    //   motorspeed_y=y;
                      speed_y = y;
                      EEPROM.put(sizeof(long), speed_y);
                    }
                    if(z > 0){
                    //   motorspeed_z=z;
                      speed_z = z;
                      EEPROM.put(2*sizeof(long), speed_z);
                    }
                    if(e > 0){
                    //   motorspeed_e=e;
                      speed_e = e;
                      EEPROM.put(3*sizeof(long), speed_e);
                    }
                    delayMicroseconds(10);
                    reply("!");
                    break;

                case 4:
                    reply("ok");
                    char message4[32];
                    sprintf(message4, "speed_x:%d", speed_x); 
                    //Serial.println(message5);
                    reply(message4); 
                    delay(10);
                    sprintf(message4, "speed_y:%d", speed_y); 
                    reply(message4);
                    //Serial.println(message5);
                    delay(10);
                    sprintf(message4, "speed_z:%d", speed_z); 
                    reply(message4);
                    //Serial.println(message5);
                    delay(10);
                    sprintf(message4, "speed_e:%d", speed_e); 
                    reply(message4);
                    //Serial.println(message5);
                    delayMicroseconds(10);
                    //reply("!");

                    break;
                 case 8:
                    // this part is only for author testing code.
                    break;
                case 9:
                    reply("ok");
                    sprintf(message4, "speed_z:%s", versionN2); 
                    reply(message4);
                    reply("!");
                    break;
                    
                default:
                    //send_to_node1("$1C99");
                    break;
            }
        }
    }
//    if (!digitalRead(switch_mode))
//    {
//            // 移动按钮的读取和逻辑
//        while (digitalRead(button_moveMax_x) == LOW) {
//            Serial.println("Moving X axis to max");
//            runMotor (motor_x_step, motor_x_dir, switch_x_endstop_max,switch_x_endstop_min, speed_x,1,1);
//        }
//
//        while (digitalRead(button_moveMin_x) == LOW) {
//            Serial.println("Moving X axis to min");
//            // 执行X轴负向移动的操作
//            runMotor (motor_x_step, motor_x_dir, switch_x_endstop_max,switch_x_endstop_min, speed_x,1,0);
//        }
//
//        while (digitalRead(button_moveMax_y) == LOW) {
//            Serial.println("Moving Y axis to max");
//            // 执行Y轴正向移动的操作
//            runMotor (motor_y_step, motor_y_dir, switch_y_endstop_max,switch_y_endstop_min, speed_y,1,1);
//        }
//
//        while (digitalRead(button_moveMin_y) == LOW) {
//            Serial.println("Moving Y axis to min");
//            // 执行Y轴负向移动的操作
//            runMotor (motor_y_step, motor_y_dir, switch_y_endstop_max,switch_y_endstop_min, speed_y,1,0);
//        }
//
//        while (digitalRead(button_moveMax_z) == LOW) {
//            Serial.println("Moving Z axis to max");
//            // 执行Z轴正向移动的操作
//            runMotor (motor_z_step, motor_z_dir, switch_z_endstop_max,switch_z_endstop_min, speed_z,1,1);
//        }
//
//        while (digitalRead(button_moveMin_z) == LOW) {
//            Serial.println("Moving Z axis to min");
//            // 执行Z轴负向移动的操作
//            runMotor (motor_z_step, motor_z_dir, switch_z_endstop_max,switch_z_endstop_min, speed_z,1,0);
//        }
//
//        while (digitalRead(button_moveMax_e) == LOW) {
//            Serial.println("Moving E axis to max");
//            // 执行E轴正向移动的操作
//            runMotor (motor_e_step, motor_e_dir, switch_e_endstop_max,switch_e_endstop_min, speed_e,1,1);
//        }
//
//        while (digitalRead(button_moveMin_e) == LOW) {
//            Serial.println("Moving E axis to min");
//            // 执行E轴负向移动的操作
//            runMotor (motor_e_step, motor_e_dir, switch_e_endstop_max,switch_e_endstop_min, speed_e,1,0);
//        }
//
//        // 复位按钮的读取和逻辑
//        if (digitalRead(button_reset_x) == LOW) {
//            Serial.println("Resetting X axis");
//            // 执行X轴复位操作
//            goToMax (motor_x_step, motor_x_dir, switch_x_endstop_max,switch_x_endstop_min, speed_x);
//        }
//
//        if (digitalRead(button_reset_y) == LOW) {
//            Serial.println("Resetting Y axis");
//            // 执行Y轴复位操作
//            goToMax (motor_y_step, motor_y_dir, switch_y_endstop_max,switch_y_endstop_min, speed_y);
//        }
//
//        if (digitalRead(button_reset_z) == LOW) {
//            Serial.println("Resetting Z axis");
//            // 执行Z轴复位操作
//            goToMax (motor_z_step, motor_z_dir, switch_z_endstop_max,switch_z_endstop_min, speed_z);
//        }
//
//        if (digitalRead(button_reset_e) == LOW) {
//            Serial.println("Resetting E axis");
//            // 执行E轴复位操作
//            goToMax (motor_e_step, motor_e_dir, switch_e_endstop_max,switch_e_endstop_min, speed_e);
//        }
//
//        if (digitalRead(button_reset_all) == LOW) {
//            Serial.println("Resetting all axes");
//            // 执行所有轴复位的操作
//            goToMax (motor_x_step, motor_x_dir, switch_x_endstop_max,switch_x_endstop_min, speed_x);
//            goToMax (motor_y_step, motor_y_dir, switch_y_endstop_max,switch_y_endstop_min, speed_y);
//            goToMax (motor_z_step, motor_z_dir, switch_z_endstop_max,switch_z_endstop_min, speed_z);
//            goToMax (motor_e_step, motor_e_dir, switch_e_endstop_max,switch_e_endstop_min, speed_e);
//        }
//
//    }
//    
}
