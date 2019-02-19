#include "FS.h"
#include "esp8266_io.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define R D0
#define G D1
#define B D2

IPAddress apIP(192, 168, 1, 114);
ESP8266WebServer webServer(80);

String httpBuff = "";
u8 delayTime;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  delayTime = 2;
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  webServer.on ("/", handleRoot);
  webServer.begin();
  SPIFFS.begin();
  File f = SPIFFS.open("/index.html", "r");
  httpBuff = f.readString();
  f.close();
}

void loop() {
  webServer.handleClient();
}

void handleRoot() {
  webServer.send(200,"text/html", httpBuff);
}

void setColor(u8 r, u8 g, u8 b) {
  analogWrite(R, map(r, 0, 255, 0, 1023));
  analogWrite(G, map(r, 0, 255, 0, 1023));
  analogWrite(B, map(r, 0, 255, 0, 1023));
}

void dazzling() {
  analogWrite(R, 1023);
  analogWrite(G, 0);
  analogWrite(B, 0);
  for(int i = 0;i < 64;i++){
    analogWrite(G, map(i ,0 , 63, 0, 1023));
    delay(delayTime);
  }
  for(int i = 64;i > 0;i--){
    analogWrite(R, map(i-1 ,0 , 63, 0, 1023));
    delay(delayTime);
  }
  for(int i = 0;i < 64;i++){
    analogWrite(B, map(i ,0 , 63, 0, 1023));
    delay(delayTime);
  }
  for(int i = 64;i > 0;i--){
    analogWrite(G, map(i-1 ,0 , 63, 0, 1023));
    delay(delayTime);
  }
  for(int i = 0;i < 64;i++){
    analogWrite(R, map(i ,0 , 63, 0, 1023));
    delay(delayTime);
  }
  for(int i = 64;i > 0;i--){
    analogWrite(B, map(i-1 ,0 , 63, 0, 1023));
    delay(delayTime);
  }
}

