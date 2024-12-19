//version

//Node03 = V1.0.0
//timeStamp = 2024-Dec-17th

String versionN2 = "Node01-V1.0.0 2024-Dec-17th";
//endstop common→GND  NO→Endstop

// 0. libraries
#include <RF24.h>
#include <EEPROM.h>

//1. role of pin
#define Z_ENDSTOP_PIN 9
#define X_STEP_PIN 2
#define Y_STEP_PIN 3
#define Z_STEP_PIN 4
#define X_DIR_PIN 5
#define Y_DIR_PIN 6
#define Z_DIR_PIN 7
#define ENABLE_PIN 8
#define X_ENDSTOP_PIN 9

#define CE A0
#define CSN A1
#define switch_forward A2 //  syringe push
#define switch_backward A3 // syringe pull(fake pull)
#define Y_ENDSTOP_PIN A4
#define switch_exit A5 // exit syring safety controlling mode


// 2. Node ID
const int nodeId = 3;

RF24 radio(CE, CSN); // CE, CSN

// 3.RF address and RF channel
const uint64_t rxAddress1 = 0xE8E8F0F0C1LL; // RX_1
const uint64_t rxAddress2 = 0xE8E8F0F0C1LL; // RX_2
const uint64_t txAddress1 = 0xE8E8F0F0A1LL; // TX_to_Node1

const uint8_t channel= 90; // RF channel

const int MAX_MESSAGE_LENGTH = 32;


// 4. Other custom variables
long x,y,z;
int motor_state;
int motorspeed_x, motorspeed_y, motorspeed_z;
long x_motor_step,y_motor_step,z_motor_step;
unsigned long startTime;
unsigned long endTime;

// 5.Custom function matchNumber 
long matchNumber(String command, String keyword) {
    int c_start = command.indexOf(keyword);
        
    if (c_start != -1) {
//        Serial.println(command);
//        Serial.println(keyword);
        
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

// 6. Custom function  reply_ok
void reply(char message_r[]){
    radio.stopListening(); // 停止接收模式，进入发送模式
    radio.openWritingPipe(txAddress1); // 设置发送地址为 txAddress1
    radio.write(message_r, strlen(message_r) + 1); // 发送消息
    radio.startListening(); // 进入接收模式
    radio.flush_rx();
    radio.flush_tx();
}

// 7. runMotor
void runMotor (int stepPin, int dirPin, int ENDSTOP_PIN, long motorspeed, long run_step) {
        digitalWrite(dirPin,HIGH); //设置顺时针旋转 set clockwise rotate
        if(run_step>0){
                           long count_step = 1;
                           while( run_step >= count_step){
                                                            int ENDSTOP_STATUS = digitalRead(ENDSTOP_PIN); //松开是High 按下是Low default is high,press is low
                                                            if(ENDSTOP_STATUS == HIGH){
                                                                                                  digitalWrite(stepPin, HIGH);
                                                                                                  delay(motorspeed);
                                                                                                  digitalWrite(stepPin, LOW);
                                                                                                  delay(motorspeed);
                                                                                                  }
                                                            else {
                                                                                                  int switch_exit_status = HIGH; //松开是high 按下是low. default is high,press is low
                                                                                                  while(switch_exit_status == HIGH){
                                                                                                                      updateMotorState();
                                                                                                                      if (digitalRead(switch_forward) || digitalRead(switch_backward)){
                                                                                                                          controlMotor(stepPin, dirPin, ENDSTOP_PIN, motorspeed, motor_state);
                                                                                                                      }
                                                                                                                      else if (switch_exit_status == LOW){
                                                                                                                                          break;
                                                                                                                      }
                                                                                                                      else{
                                                                                                                            delay(10);//节省计算资源 save calculation efficency
                                                                                                                      }
                                                                                                     switch_exit_status = digitalRead(switch_exit);
                                                                                                  }
                                                                                                  break;
                                                            }
                                                            count_step = count_step+1;
                            
                           }
        }
}
void control_x_by_button (){
   int switch_exit_status = HIGH; //松开是high 按下是low
            while(switch_exit_status == HIGH){
                                updateMotorState();
                                if (digitalRead(switch_forward) || digitalRead(switch_backward)){
                                    controlMotor(2, 5, 9, 10, motor_state);
                                }
                                else if (switch_exit_status == LOW){
                                                    break;
                                }
                                else{
                                      delay(10);//节省计算资源
                                }
               switch_exit_status = digitalRead(switch_exit);
            }
}
void control_y_by_button (){
   int switch_exit_status = HIGH; //松开是high 按下是low
            while(switch_exit_status == HIGH){
                                updateMotorState();
                                if (digitalRead(switch_forward) || digitalRead(switch_backward)){
                                    controlMotor(3, 6, A4, 10, motor_state);
                                }
                                else if (switch_exit_status == LOW){
                                                    break;
                                }
                                else{
                                      delay(10);//节省计算资源
                                }
               switch_exit_status = digitalRead(switch_exit);
            }
}
void control_z_by_button (){
   int switch_exit_status = HIGH; //松开是high 按下是low
            while(switch_exit_status == HIGH){
                                updateMotorState();
                                if (digitalRead(switch_forward) || digitalRead(switch_backward)){
                                    controlMotor(4, 7, 1, 10, motor_state);
                                }
                                else if (switch_exit_status == LOW){
                                                    break;
                                }
                                else{
                                      delay(10);//节省计算资源
                                }
               switch_exit_status = digitalRead(switch_exit);
            }
}
void controlMotor(int step_num, int dir_num, int end_stop, long motorspeed, int motor_state_s) {
    //是为了手控motor的function
    //this function is for hand control motor
    switch (motor_state_s){
        case 1:
            // 正转状态
            motorspeed = 1;
            digitalWrite(dir_num, LOW);
            digitalWrite(step_num, HIGH);
            delay(motorspeed);
            digitalWrite(step_num, LOW);
            delay(motorspeed);
            break;
           

        case 2:
            // 反转状态
            motorspeed = 5;
            digitalWrite(dir_num, HIGH);
            digitalWrite(step_num, HIGH);
            delay(motorspeed);
            digitalWrite(step_num, LOW);
            delay(motorspeed);
            break;

        case 0:
            delay(10);
    }
}

void updateMotorState() {
    if (digitalRead(switch_forward) == LOW) {
        // 按下开关1，正转
        
        motor_state = 1;
    } 
    
    else if (digitalRead(switch_backward) == LOW) {
        // 按下开关2，反转
        motor_state = 2;
        
    } 
    else{
      motor_state = 0;
    }
}

// 10. Custom function delete_motor_time
void delete_motor_time(){
 x_motor_step=-1;
 y_motor_step=-1;
 z_motor_step =-1;
}


// 11.初始化
void setup() {

  pinMode(X_STEP_PIN,OUTPUT);
  pinMode(X_DIR_PIN,OUTPUT);
  pinMode(Y_STEP_PIN,OUTPUT);
  pinMode(Y_DIR_PIN,OUTPUT);
  pinMode(Z_STEP_PIN,OUTPUT);
  pinMode(Z_DIR_PIN,OUTPUT);

  pinMode(X_ENDSTOP_PIN, INPUT_PULLUP);
  pinMode(Y_ENDSTOP_PIN, INPUT_PULLUP);
  pinMode(Z_ENDSTOP_PIN, INPUT_PULLUP);// X_ENDSTOP_PIN
  pinMode(switch_exit, INPUT_PULLUP);
  pinMode(switch_forward, INPUT_PULLUP);    // switch_forward
  pinMode(switch_backward, INPUT_PULLUP);    // switch_backward
  
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);

  motorspeed_x = EEPROM.get(0, motorspeed_x);
  motorspeed_y =  EEPROM.get(sizeof(long), motorspeed_y);
  motorspeed_z =  EEPROM.get(2*sizeof(long), motorspeed_z);

  delete_motor_time();

  radio.begin();                        // Setup and configure rf radio
  radio.setChannel(channel);            // Set the channel
  radio.openWritingPipe(txAddress1);         // Open the default reading and writing pipe
  radio.openReadingPipe(1, rxAddress1);
  radio.setPALevel(RF24_PA_MAX);        // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
  radio.setDataRate(RF24_2MBPS);
  radio.setAutoAck(1);                  // Ensure autoACK is enabled
  radio.setRetries(15, 15);             // Optionally, increase the delay between retries. Want the number of auto-retries as high as possible (15)
  radio.setCRCLength(RF24_CRC_16);      // Set CRC length to 16-bit to ensure quality of data
  radio.startListening();               // Start listening
  radio.powerUp();                      //Power up the radio

    // 打开串口 波特率115200
  Serial.begin(115200);
  Serial.println("ok");
}

// 12. loop函数
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
            //Serial.print("c = ");
            //Serial.println(c);
            switch (c) {

                case 0:
                    reply("ok");
                    //Serial.println("case 0 RE");
                    char message4[32]; // 创建一个足够大的字符数组来保存消息
                    strcpy(message4, "Robot3 Node3 for PUMP1-4");
                    reply(message4); // 发送消息
                    delayMicroseconds(10);
                    strcpy(message4, "c1: Reply ok");
                    reply(message4); // 发送消息
                    delayMicroseconds(10);
                    //message4 = "c2: Run motor, step";
                    strcpy(message4, "c2: Run motor, step");
                    reply(message4);
                    delayMicroseconds(10);
                    //message4 = "c3: Set motor speed and Write into EEPROM";
                    strcpy(message4, "c3: Set motor speed");
                    reply(message4);
                    delayMicroseconds(10);
                    //message4 = "c4: Ask motor speed";
                    strcpy(message4, "c4: Ask motor speed");
                    reply(message4);
                    delayMicroseconds(10);
                    strcpy(message4, "c5: Hand Control X");
                    reply(message4);
                    delayMicroseconds(10);
                    strcpy(message4, "c6: Hand Control Y");
                    reply(message4);
                    delayMicroseconds(10);
                    strcpy(message4, "c7: Hand Control Z");
                    reply(message4);
                    delayMicroseconds(10);
                    strcpy(message4, "c8: For TESTING Only");
                    reply(message4);
                    delayMicroseconds(10);
                    strcpy(message4, "c9: coding version");
                    reply(message4);
                    //delayMicroseconds(10);
                    reply("!");
                    radio.flush_rx();
                    radio.flush_tx();
                    break;

                case 1:
                    
                    reply("ok");
                    //Serial.println("case 1 RE");
                    delayMicroseconds(10);
                    reply("!");
                    break;

                case 2:
                    //Serial.println("case 2 RE");
                    reply("ok");
                    x_motor_step= matchNumber(message, "x");
                    y_motor_step = matchNumber(message, "y");
                    z_motor_step = matchNumber(message, "z");
                    delayMicroseconds(50);
                    
                    if(x_motor_step > 0){
                       runMotor (X_STEP_PIN, X_DIR_PIN, X_ENDSTOP_PIN, motorspeed_x, x_motor_step);
                    }
                    if(y_motor_step > 0){
                      runMotor (Y_STEP_PIN, Y_DIR_PIN, X_ENDSTOP_PIN, motorspeed_y, y_motor_step);
                    }
                    if(z_motor_step > 0){
                       runMotor (Z_STEP_PIN, Z_DIR_PIN, Z_ENDSTOP_PIN, motorspeed_z, z_motor_step);
                    }
                    
                    delete_motor_time();
                    delayMicroseconds(10);
                    
                    reply("!");
                    break;

                case 3:
                    reply("ok");
                    //Serial.println("case 3 RE");
                    x = matchNumber(message, "x");
                    y = matchNumber(message, "y");
                    z = matchNumber(message, "z");
                    if (x > 0){
                      motorspeed_x=x;
                      Serial.println(motorspeed_x);
                      EEPROM.put(0, motorspeed_x);
                    }

                    if(y > 0){
                      motorspeed_y=y;
                      EEPROM.put(sizeof(unsigned long), motorspeed_y);
                    }
                    if(z > 0){
                      motorspeed_z=z;
                      EEPROM.put(2*sizeof(unsigned long), motorspeed_z);
                    }
                    x = -1 ;
                    y = -1 ;
                    z = -1 ;
                  delayMicroseconds(10);
                    reply("!");
                    break;

                case 4:
                  
                    //Serial.println("case 4 RE");
                    reply("ok");
                    char message5[32]; // 创建一个足够大的字符数组来保存消息
                    sprintf(message5, "speed_x:%d", motorspeed_x); // 将速度值转换为字符串并拼接
                    Serial.println(message5);
                    reply(message5); // 发送消息
                    delay(10);
                    sprintf(message5, "speed_y:%d", motorspeed_y); // 将速度值转换为字符串并拼接
                    reply(message5);
                    Serial.println(message5);
                    delay(10);
                    sprintf(message5, "speed_z:%d", motorspeed_z); // 将速度值转换为字符串并拼接
                    reply(message5);
                    Serial.println(message5);
                    delayMicroseconds(10);
                    reply("!");
                  
                    break;
                
                 case 5:
                    reply("ok");
                    control_x_by_button ();
                    reply("!");
                    break;

                case 6:
                    reply("ok");
                    control_y_by_button ();
                    reply("!");
                    break;
                case 7:
                    reply("ok");
                    control_z_by_button ();
                    reply("!");
                    break;
                    
                 case 8:
                    // this part is only for author testing code.
                    char testMessage[50]; 
                    x = digitalRead(X_ENDSTOP_PIN);
                    
                    y = digitalRead(Y_ENDSTOP_PIN);
                    
                    z = digitalRead(Z_ENDSTOP_PIN);
                    
                     // 格式化并发送x轴速度
                    sprintf(testMessage, "x is%ld", x); 

                    
                    reply(testMessage);
                
                    // 格式化并发送y轴速度
                    sprintf(testMessage, "y is%ld", y); 
                    reply(testMessage);
                
                    // 格式化并发送z轴速度
                    sprintf(testMessage, "z is%ld", z); 
                    reply(testMessage);
                    break;
                case 9:
                    reply("ok");
                    versionN2.toCharArray(testMessage, sizeof(testMessage));
                    reply(testMessage);
                    reply("!");
                    break;
                    
                default:
                    //send_to_node1("$1C99");
                    break;
            }
        }
    }
}
