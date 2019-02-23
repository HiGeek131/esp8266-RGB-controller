#include "FS.h"
#include "esp8266_io.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "EEPROM.h"

#define R D0
#define G D1
#define B D2

IPAddress apIP(192, 168, 1, 1);
ESP8266WebServer webServer(80);
String httpBuff = "";
u8 delayTime = 2;

u8 color0Num,color1Num;

u8 colorGroup0[21][3];
u8 colorGroup1[21][3];

void setup()
{
  Serial.begin(115200);
  Serial.println("");

  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  // eeprom初始化
  EEPROM.begin(256);
  // rgb参数获取
  getRGBValues();
  //WiFi设置
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(eepromReadStr(0x00), eepromReadStr(0x10), 10, 0, 2);

  Serial.print("[Debug]->软AP WiFi已启动, SSID: ");
  Serial.print(WiFi.softAPSSID());
  Serial.print(", IP地址: ");
  Serial.println(WiFi.softAPIP());
  //Web服务器设置
  webServer.on("/", handleRoot);         //根文件
  webServer.on("/update", handleUpdate); //更新
  webServer.begin();
  Serial.print("[Debug]->Web服务器已启动, IP: ");
  Serial.print(WiFi.softAPIP());
  Serial.print(", 端口:");
  Serial.println(80);
  //SPIFFS设置
  SPIFFS.begin();
  Serial.println("[Debug]->SPIFFS已加载完毕");
}

/* *
 * 循环
 * */
void loop()
{
  webServer.handleClient();
}

/* *
 * Web服务器事件
 * */
void handleRoot()
{
  Serial.println("[Debug]->Web访问: /");
  File f = SPIFFS.open("/index.html", "r");
  httpBuff = f.readString();
  f.close();
  webServer.send(200, "text/html", httpBuff);
}

void handleUpdate()
{
  Serial.println("[Debug]->Web访问: /update");
  String str = webServer.arg("plain");
  Serial.print("[Debug]->Web Post:");
  Serial.println(str);
  //Parse JSON数据
  const size_t capacity = JSON_OBJECT_SIZE(3) + 30;
  DynamicJsonDocument jsonBuffer(capacity);
  DeserializationError error = deserializeJson(jsonBuffer, str);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  JsonObject root = jsonBuffer.as<JsonObject>();

  if (!root.containsKey("red") || !root.containsKey("green") || !root.containsKey("blue"))
  {
    webServer.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Bad Input\"}");
  }
  else
  {
    setColor(root["red"], root["green"], root["blue"]);//设置
    Serial.println(str);
    webServer.send(200, "application/json", str);
  }
}

/**
 * r: 8bit 红色
 * g: 8bit 绿色
 * b: 8bit 蓝色
 */
void setColor(u8 r, u8 g, u8 b)
{
//测试
  analogWrite(R, map(r, 0, 255, 0, 1023));
  analogWrite(G, map(g, 0, 255, 0, 1023));
  analogWrite(B, map(b, 0, 255, 0, 1023));

//  成品使用  
//  analogWrite(R, map(r, 0, 255, 1023, 0));
//  analogWrite(G, map(g, 0, 255, 1023, 0));
//  analogWrite(B, map(b, 0, 255, 1023, 0));
}

void dazzling()
{
  analogWrite(R, 1023);
  analogWrite(G, 0);
  analogWrite(B, 0);
  for (int i = 0; i < 64; i++)
  {
    analogWrite(G, map(i, 0, 63, 1023, 0));
    delay(delayTime);
  }
  for (int i = 64; i > 0; i--)
  {
    analogWrite(R, map(i - 1, 0, 63, 1023, 0));
    delay(delayTime);
  }
  for (int i = 0; i < 64; i++)
  {
    analogWrite(B, map(i, 0, 63, 1023, 0));
    delay(delayTime);
  }
  for (int i = 64; i > 0; i--)
  {
    analogWrite(G, map(i - 1, 0, 63, 1023, 0));
    delay(delayTime);
  }
  for (int i = 0; i < 64; i++)
  {
    analogWrite(R, map(i, 0, 63, 1023, 0));
    delay(delayTime);
  }
  for (int i = 64; i > 0; i--)
  {
    analogWrite(B, map(i - 1, 0, 63, 1023, 0));
    delay(delayTime);
  }
}
/* *
 * 读取一串数据
 * */
u8 *eepromRead(u16 addr, u16 num) {
  u8 *p;
  for(u16 i = 0; i < num; i++) {
    p[i] = EEPROM.read(addr+i);
  }
  return p;
}
/* *
 * 读取字符串
 * */
String eepromReadStr(u16 addr) {
  String str;
  char buff;
  for(u16 i = 0; i < 16; i++) {
    buff = EEPROM.read(addr+i);
    if (buff =='\0')
      return str;
    str += buff;
  }
  return "ERROR";
}
/* *
 * 写入一串数据
 * */
void eepromWrite(u16 addr, u8 *p, u16 num) {
  for(u16 i = 0; i < num; i++) {
    EEPROM.write(addr+i,p[i]);
  }
  EEPROM.commit();
}
/* *
 * 写入字符串
 * */
void eepromWriteStr(u16 addr, String str) {
  u16 numToWrite = str.length()+1;
  for(u16 i = 0; i < numToWrite; i++) {
    EEPROM.write(addr+i,str[i]);
  }
  EEPROM.commit();
}
/* *
 * 读取两组RGB数据
 * */
void getRGBValues() {
  u8 buff[64];
  for(u8 i = 0; i < 64; i++) {
    buff[0] = EEPROM.read(0x20+i);
  }
  color0Num = buff[63];
  for(u8 i = 0; i < color0Num; i++) {
    colorGroup0[i][0] = buff[i*3];
    colorGroup0[i][1] = buff[i*3+1];
    colorGroup0[i][2] = buff[i*3+2];
  }
  for(u8 i = 0; i < 64; i++) {
    buff[0] = EEPROM.read(0x60+i);
  }
  color1Num = buff[63];
  for(u8 i = 0; i < color0Num; i++) {
    colorGroup1[i][0] = buff[i*3];
    colorGroup1[i][1] = buff[i*3+1];
    colorGroup1[i][2] = buff[i*3+2];
  }
}
