//Node01 = V1.0
//timeStamp = 2024-Dec-17th
#include <RF24.h>

const int nodeId = 1; 
// 初始化 RF24 模块
RF24 radio(9, 10); // CE, CSN
const uint64_t node1A = 0xE8E8F0F0A1LL; // 监听地址1：接收Node2消息用
const uint64_t node1B = 0xE8E8F0F0A2LL; // 监听地址2：接收Node3消息用

const uint64_t node2 = 0xE8E8F0F0B1LL; // 发送 Node2 B1
const uint64_t node3 = 0xE8E8F0F0C1LL; // 发送 Node2 B1
const uint64_t node4 = 0xE8E8F0F0D1LL; // 发送 Node2 B1
const uint64_t node5 = 0xE8E8F0F0E1LL; // 发送 Node2 B1
const uint64_t node6 = 0xE8E8F0F0F1LL; // 发送 Node2 B1
const uint8_t channel_A1_90=90; // 射频通道

const int MAX_MESSAGE_LENGTH = 64;

int matchNumber(String command, String keyword) {
  int start = command.indexOf(keyword);
  if (start != -1) {
    int end = command.indexOf(" ", start);
    String num = command.substring(start + keyword.length(), end);
    return num.toInt();
  }
  return -1;
}

void setup() {
  Serial.begin(115200);

  // 配置 RF 模块
  radio.begin();
  radio.setChannel(channel_A1_90);
  radio.openWritingPipe(node2); // 默认为Node2的地址
  radio.openReadingPipe(1, node1A);
  radio.openReadingPipe(2, node1B);
  radio.setPALevel(RF24_PA_MAX);
  radio.setRetries(15, 15);
  radio.startListening();
  
  Serial.println("RemoteNode:"); 
  Serial.println("$2c0");
  Serial.println("$3c0");
  Serial.println("$4c0");
  Serial.println("$5c0");
  Serial.println("$6c0");
}

void loop() {
  // 从串口接收数据并添加到队列
  if (Serial.available()) {
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
          radio.write(message, strlen(message) + 1); // Include the null-terminator
          radio.startListening();
          radio.flush_rx();
          radio.flush_tx();
}
  // 从 RF 接收数据并通过串口回传给电脑
  if (radio.available()) {
      char message[MAX_MESSAGE_LENGTH + 1];
      radio.read(message, MAX_MESSAGE_LENGTH + 1);
      message[MAX_MESSAGE_LENGTH] = '\0'; // Make sure the string is null-terminated
      Serial.println(message);
  }
}
