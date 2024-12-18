 //Node02 = V1.0
//timeStamp = 2024-Dec-17th

char nodeVersion[] = "Node02 version1.0.1";

// 0. 第三方库
#include <SPI.h>
#include <RF24.h>
#include <AccelStepper.h>

// 1. 节点的ID
const int nodeId = 2; // 子节点的ID
int x,y,z,r,n,s;
// r is better to try with 100 first and then gradually increse 100 each time
//n is better to smaller than 10, best to choose number 1-3
// 2. 初始化 RF24 模块
RF24 radio(A0, A1); // CE, CSN
const uint64_t rxAddress1 = 0xE8E8F0F0B1LL; // 监听地址1：接收Node2消息用
const uint64_t rxAddress2 = 0xE8E8F0F0B2LL; // 监听地址2：接收Node3消息用

const uint64_t txAddress1 = 0xE8E8F0F0A1LL; // 发送 Node1
const uint64_t txAddress2 = 0xE8E8F0F0C1LL; // 发送 Node3
const uint8_t channel = 90; // 射频通道


const int MAX_MESSAGE_LENGTH = 32;

// 4. 电机引脚
#define X_DIR_PIN 5
#define X_STEP_PIN 2
#define X_ENDSTOP_PIN 9

#define Y_DIR_PIN 6
#define Y_STEP_PIN 3

#define Y_ENDSTOP_PIN A3

#define Z_DIR_PIN 7
#define Z_STEP_PIN 4
#define Z_ENDSTOP_PIN 10

// 5. 初始化AccelStepper对象
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper stepperY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
AccelStepper stepperZ(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);

// 6. 自定义函数 matchNumber 搜索字符串中的关键参数并赋值给相应变量
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

void dispenser_home(){
  int delay_time = 1000000 / 700;
    digitalWrite(Z_DIR_PIN, LOW);
    while (digitalRead(Z_ENDSTOP_PIN)) {
        digitalWrite(Z_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(Z_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
    }
    
  // 稍微暂停
    delay(1000);

    // z慢速向正方向移动，直到离开限位开关
    delay_time = 1000000 / 1000; // 500 是慢速移动的速度
    digitalWrite(Z_DIR_PIN, HIGH);
    
    while (!digitalRead(Z_ENDSTOP_PIN)) {
        digitalWrite(Z_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(Z_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
        }
    
}

void dispenser_go(){
   int extra_steps = 3000;
    int delay_time = 1000000 / 1500;
    int steps_counter = 0;
    digitalWrite(Z_DIR_PIN, HIGH);
    while (steps_counter < extra_steps) {
        digitalWrite(Z_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(Z_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
        steps_counter++;
        }
}
void home_stepper_direct() { 
    // 设置一个速度进行归零
    int delay_time = 1000000 / 500; // 2000 是步进电机的速度
    int extra_steps = 100; // 设置额外的步数，可以根据需要调整
    
    // x向负方向移动，直到触发限位开关
    digitalWrite(X_DIR_PIN, LOW);
    while (digitalRead(X_ENDSTOP_PIN)) {
        digitalWrite(X_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(X_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
    }
    
  // 稍微暂停
    delay(1000);

    // x慢速向正方向移动，直到离开限位开关
    delay_time = 1000000 / 500; // 500 是慢速移动的速度
    digitalWrite(X_DIR_PIN, HIGH);
    
    while (!digitalRead(X_ENDSTOP_PIN)) {
        digitalWrite(X_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(X_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
        }
    
    int steps_counter = 0;
    while (steps_counter < extra_steps) {
        digitalWrite(X_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(X_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
        steps_counter++;
        }
    
      // 稍微暂停
    delay(1000);

  // y向负方向移动，直到触发限位开关
    delay_time = 1000000 / 500; // 1000 是步进电机的速度
    digitalWrite(Y_DIR_PIN, LOW);
    while (digitalRead(Y_ENDSTOP_PIN)) {
        digitalWrite(Y_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(Y_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
    }
    delay(1000);
    
  // y慢速向正方向移动，直到离开限位开关
    delay_time = 1000000 / 500;
    digitalWrite(Y_DIR_PIN, HIGH);

    while (!digitalRead(Y_ENDSTOP_PIN)) {
        digitalWrite(Y_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(Y_STEP_PIN, LOW);
        delayMicroseconds(delay_time); 
    }
        
    steps_counter = 0;
    while (steps_counter < extra_steps) {
        digitalWrite(Y_STEP_PIN, HIGH);
        delayMicroseconds(delay_time);
        digitalWrite(Y_STEP_PIN, LOW);
        delayMicroseconds(delay_time);
        steps_counter++;
        }
}
  
// 10.自定义函数 响应ok给node1
void reply(char message_r[]) {
    radio.stopListening(); // 停止接收模式，进入发送模式
    radio.openWritingPipe(txAddress1); // 设置发送地址为 txAddress1
    radio.write(message_r, strlen(message_r) + 1); // 发送消息
    radio.startListening(); // 进入接收模式
    radio.flush_rx();
    radio.flush_tx();
}

// 11.自定义函数 移动到坐标
void moveXYPosition(int x_loc, int y_loc) {
      // 将电机移动到目标位置
      if (x_loc>=0){
        stepperX.moveTo(x_loc);
      }
      if (y_loc>=0){
        stepperY.moveTo(y_loc);
      }
      while (stepperX.distanceToGo() != 0 || stepperY.distanceToGo() != 0) {
        stepperX.run();
        stepperY.run();
      }
}

void moveZPosition(int z_loc) {
      // 将电机移动到目标位置
      if (z_loc>=0){
        stepperZ.moveTo(z_loc);
      }
      while (stepperZ.distanceToGo() != 0 ) {
        stepperZ.run();
      }
}

// 13.自定义函数 发消息给node1
void send_to_node(char message3[], const uint64_t address){
    radio.stopListening(); // 停止接收模式，进入发送模式
    radio.openWritingPipe(address); // 设置发送地址为 txAddress2
    radio.write(message3, strlen(message3) + 1); // 发送消息
    radio.startListening(); // 进入接收模式
}
//14.Restart RF24
void reinitializeRF24() {
    // 停止接收模式
    radio.stopListening();
    // 关闭nRF24L01模块
    radio.powerDown();
    // 等待一段时间，例如100毫秒
    delay(100);
    // 打开nRF24L01模块
    radio.powerUp();
    // 重新配置RF模块
    radio.begin();
    radio.setChannel(channel);
    radio.openWritingPipe(txAddress1); // 默认为Node2的地址
    radio.openReadingPipe(1, rxAddress1);
    radio.setPALevel(RF24_PA_LOW); // 设置为最大功率等级
    radio.startListening();
    // 输出日志，表示重新初始化完成
    delayMicroseconds(100);
    //Serial.println("RF24 module reinitialized.");
}

void moveInCircle(float radius, int circle_num, int currentX, int currentY, int steps) {
//  int steps = 90; // 每圈分为360步
  float angleIncrement = 2 * PI / steps;
  
  for (int j = 0; j < circle_num; j++) {
    for (int i = 0; i < steps; i++) {
      float angle = i * angleIncrement;
      float targetX = currentX + radius * cos(angle);
      float targetY = currentY + radius * sin(angle);
      stepperX.moveTo(targetX);
      stepperY.moveTo(targetY);
      while (stepperX.distanceToGo() != 0 || stepperY.distanceToGo() != 0) {
          stepperX.run();
          stepperY.run();
        }
    }
  }
}

// 15. SETUP
void setup() {
  // 7.3 启动电机工作和配制endstop
    pinMode(8, OUTPUT);
    digitalWrite(8, LOW);

    pinMode(X_ENDSTOP_PIN, INPUT_PULLUP);
    pinMode(Y_ENDSTOP_PIN, INPUT_PULLUP);
    pinMode(Z_ENDSTOP_PIN, INPUT_PULLUP);

    // 7.4 配置电机和限位开关
    stepperX.setMaxSpeed(2000);
    stepperY.setMaxSpeed(2000);
    stepperX.setAcceleration(500);
    stepperY.setAcceleration(500);

    stepperZ.setMaxSpeed(2000);
    stepperZ.setAcceleration(500);

    // 7.5 重新调零
    //home_stepper_direct();
      
  // 将当前位置设置为零点（在调用此函数后，您需要为相应的AccelStepper实例设置零点）
    //stepperX.setCurrentPosition(0);
    //stepperY.setCurrentPosition(0);
    
  Serial.begin(115200);

  radio.begin();                        // Setup and configure rf radio
  radio.setChannel(channel);            // Set the channel
  radio.openWritingPipe(txAddress1);         // Open the default reading and writing pipe
  radio.openReadingPipe(1, rxAddress1);
  radio.setPALevel(RF24_PA_MAX);        // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
  radio.setAutoAck(1);                  // Ensure autoACK is enabled
  radio.setRetries(15, 15);             // Optionally, increase the delay between retries. Want the number of auto-retries as high as possible (15)
  radio.setCRCLength(RF24_CRC_16);      // Set CRC length to 16-bit to ensure quality of data
  radio.startListening();               // Start listening
  radio.powerUp();                      //Power up the radio
}
// 16. LOOP
void loop() {
  // 从 RF 接收数据并通过串口回传给电脑
  if (radio.available()) {
      char message[MAX_MESSAGE_LENGTH + 1];
      radio.read(message, MAX_MESSAGE_LENGTH + 1);
      message[MAX_MESSAGE_LENGTH] = '\0'; 
      int targetNodeId = matchNumber(message, "$"); 
      if(targetNodeId == nodeId){
          int c = matchNumber(message,"c");
          switch (c) {
          
              case 0:
                  reply("ok");
                  char message4[32]; // 创建一个足够大的字符数组来保存消息
                  strcpy(message4, "Node2 XY-Motion");
                  send_to_node(message4,txAddress1); // 发送消息
                  delayMicroseconds(10);
                  
                  strcpy(message4, "1: ReplyOK");
                  send_to_node(message4,txAddress1); // 发送消息
                  delayMicroseconds(10);

                  // strcpy(message4, "4: moveXY 6:zHome");
                  // send_to_node(message4,txAddress1); // 发送消息
                  // delayMicroseconds(10);
                  
                  strcpy(message4, "2: GoHome");
                  send_to_node(message4,txAddress1);
                  delayMicroseconds(10);

                  strcpy(message4, "3: z go");
                  send_to_node(message4,txAddress1);
                  delayMicroseconds(10);

                  strcpy(message4, "4: MoveXY");
                  send_to_node(message4,txAddress1);
                  delayMicroseconds(10);

                  strcpy(message4, "5: CircleXY(key:r,n,s)");
                  send_to_node(message4,txAddress1);
                  delayMicroseconds(10);

                  strcpy(message4, "6: dispenser home");
                  send_to_node(message4,txAddress1);
                  delayMicroseconds(10);

                  strcpy(message4, "7: dispenser go");
                  send_to_node(message4,txAddress1);
                  delayMicroseconds(10);
                  
                  reply("!");
      
                  radio.flush_tx();
                  radio.flush_rx();
                  break;
              
              case 1:
                  reply("ok");
                  delayMicroseconds(100);
                  reply("!");
                  break;
      
              case 2:
                reply("ok");
                dispenser_home();
                stepperZ.setCurrentPosition(0);
                home_stepper_direct();
                  // 将当前位置设置为零点（在调用此函数后，您需要为相应的AccelStepper实例设置零点）
                stepperX.setCurrentPosition(0);
                stepperY.setCurrentPosition(0);
                //dispenser_home();
                delayMicroseconds(10);
                reply("!");
                break;
                
              case 3:
                reply("ok");
                z = matchNumber(message,"z"); 
                moveZPosition(z);
                delayMicroseconds(10);
                reply("!");
                break;
              
              case 4:
                reply("ok");
                x = matchNumber(message,"x"); 
                y = matchNumber(message,"y"); 
                moveXYPosition(x, y);
                delayMicroseconds(10);
                reply("!");
                break;
              
              case 5:
                reply("ok");
                x = stepperX.currentPosition();
                y = stepperY.currentPosition();
                r = matchNumber(message,"r"); 
                n = matchNumber(message,"n"); 
                s = matchNumber(message,"s"); //how many step per cicle 
                moveInCircle(r,n,x,y,s);
                moveXYPosition(x, y);
                delayMicroseconds(10);
                reply("!");
                break;

              case 6:
                  reply("ok");
                  dispenser_home();
                  stepperZ.setCurrentPosition(0);
                  delayMicroseconds(10);
                  reply("!");
                  break;
//              
              case 7:
                  reply("ok");
                  moveZPosition(3000);
                  delayMicroseconds(10);
                  reply("!");
                  break;

              default:
                  reply("ok");
                  reply(nodeVersion);
                  reply("!");
                  break;
          }
      }

  }
 
}
