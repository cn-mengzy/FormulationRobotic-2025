//Node01 = V1.0.1
//timeStamp = 2024-Dec-17th
#include <RF24.h>

const int nodeId = 1; 

RF24 radio(9, 10);                      // 初始化 RF24 模块
const uint64_t node1A = 0xE8E8F0F0A1LL; // 监听地址1：接收Node2消息用
const uint64_t node1B = 0xE8E8F0F0A2LL; // 监听地址2：接收Node3消息用

const uint64_t node2 = 0xE8E8F0F0B1LL; 
const uint64_t node3 = 0xE8E8F0F0C1LL; 
const uint64_t node4 = 0xE8E8F0F0D1LL; 
const uint64_t node5 = 0xE8E8F0F0E1LL; 
const uint64_t node6 = 0xE8E8F0F0F1LL; 
const uint8_t channel=90; 

const int MAX_MESSAGE_LENGTH = 64;

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

void setup() {
  Serial.begin(115200);
  Serial.println("RemoteNode:"); 
  Serial.println("$2c0");
  Serial.println("$3c0");
  Serial.println("$4c0");
  Serial.println("$5c0");
  Serial.println("$6c0");

  radio.begin();                        // Setup and configure rf radio
  radio.setChannel(channel);            // Set the channel
  radio.openWritingPipe(node2);         // Open the default reading and writing pipe
  radio.openReadingPipe(1, node1A);
  radio.openReadingPipe(2, node1B);
  radio.setPALevel(RF24_PA_MAX);        // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
  radio.setAutoAck(1);                  // Ensure autoACK is enabled
  radio.setRetries(15, 15);             // Optionally, increase the delay between retries. Want the number of auto-retries as high as possible (15)
  radio.setCRCLength(RF24_CRC_16);      // Set CRC length to 16-bit to ensure quality of data
  radio.startListening();               // Start listening
  radio.powerUp();                      //Power up the radio
}

void loop() {
 
  if (Serial.available()) {                                                             // 从串口接收数据并添加到队列
      char message[MAX_MESSAGE_LENGTH + 1];
      size_t bytesRead = Serial.readBytesUntil('\n', message, MAX_MESSAGE_LENGTH);
      message[bytesRead] = '\0'; // Add null-terminator

      int targetNodeId = matchNumber(message, "$");
      
      if (targetNodeId == 2){
          radio.openWritingPipe(node2);
      }
      else if (targetNodeId == 3){
          radio.openWritingPipe(node3);
      }
      else if (targetNodeId == 4){
          radio.openWritingPipe(node4);
      }
      else if (targetNodeId == 5){
          radio.openWritingPipe(node5);
      }
      else if (targetNodeId == 6){
          radio.openWritingPipe(node6);
      }
          radio.stopListening();
          radio.write(message, strlen(message) + 1);                                    // Include the null-terminator
          radio.startListening();
          radio.flush_rx();
          radio.flush_tx();
}
  if (radio.available()) {                                                              // 从 RF 接收数据并通过串口回传给电脑
      char message[MAX_MESSAGE_LENGTH + 1];
      radio.read(message, MAX_MESSAGE_LENGTH + 1);
      message[MAX_MESSAGE_LENGTH] = '\0';                                               // Make sure the string is null-terminated
      Serial.println(message);
  }
}
