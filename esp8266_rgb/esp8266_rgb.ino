#include "FS.h"
#include "esp8266_io.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define R D0
#define G D1
#define B D2

IPAddress apIP(192, 168, 1, 114);
ESP8266WebServer webServer(80);
String httpBuff = "";
u8 delayTime;
//Wifi配置
const char *SSID = "ESP8266WiFi";

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("");
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  delayTime = 2;
  //WiFi设置
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID);
  Serial.print("[Debug]->软AP WiFi已启动, SSID: ");
  Serial.print(SSID);
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

/**
 * 循环
 * */
void loop()
{
  webServer.handleClient();
}

/**
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
