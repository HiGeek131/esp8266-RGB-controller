#include "FS.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "EEPROM.h"

#define R 16
#define G 5
#define B 4
#define KEY 2

IPAddress apIP(192, 168, 1, 1);
ESP8266WebServer webServer(80);
String SSID,passwd;
String httpBuff = "";

// json buffer size
DynamicJsonDocument jsonBuffer(10240);

// 每个模式下的颜色个数
u8 mode0Num = 15, mode1Num;

// 每个模式下的颜色缓存
u8 colorGroup0[21][3];  // 模式一可以储存21种颜色
u8 colorGroup1[25][5];  // 模式二可以储存25种颜色，前三位为颜色信息，后两位为延时信息

// 每个模式下必须的全局变量
u16 mode0Speed = 100; // 爆闪速度，此为半周期延时，单位为毫秒。
u8 mode0BlinkFlag;  // 爆闪标志位
u8 mode2Speed = 2;  // 滚动速度，此为滚动速度。
u8 mode0ColorFlag, mode1ColorFlag; // 此为当前模式下颜色所处于的位数。

// 模式标志位
u8 modeFlag = 0;

// 按键辅助标志位
u32 timeKey;   // 双击间隔时间辅助位

void setup()
{
  Serial.begin(115200);
  Serial.println("123");

  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(KEY, INPUT);
  Serial.println("IO");
  // eeprom初始化
  EEPROM.begin(256);
  Serial.println("EEPROM");
//  getRGBValues();
  Serial.println("READ");
  SSID = eepromReadStr(0x00);
  Serial.println("READ");
  passwd = eepromReadStr(0x10);
  Serial.println("READ");
  //WiFi设置
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID, passwd, 6, 0, 2);
  Serial.println("WIFI");

  Serial.print("[Debug]->软AP WiFi已启动, SSID: ");
  Serial.print(WiFi.softAPSSID());
  Serial.print(", IP地址: ");
  Serial.println(WiFi.softAPIP());
  //SPIFFS设置
  SPIFFS.begin();
  Serial.println("[Debug]->SPIFFS已加载完毕");
  //Web服务器设置
  webServer.on("/", handleRoot);         //根文件
  webServer.on("/update", handleUpdate); //更新
  Serial.println("server");
  webServer.begin();
  Serial.print("[Debug]->Web服务器已启动, IP: ");
  Serial.print(WiFi.softAPIP());
  Serial.print(", 端口:");
  Serial.println(80);
  setColor(0, 0, 0);

  attachInterrupt(KEY, keyIntt, FALLING);

  settingsInit();
}

/* *
 * 循环
 * */
void loop()
{
  webServer.handleClient();
  Serial.println(modeFlag);
  switch (modeFlag)
  {
    case 0:
      mode0();
      break;

    case 1:
      mode1();
      break;

    case 2:
      mode2();
      break;
  
    default:
      break;
  }
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
  DeserializationError error = deserializeJson(jsonBuffer, str);
  if (error) {
    Serial.print("deserializeJson() failed: ");
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
void keyIntt() {
  delay(50);
  if (digitalRead(KEY) == LOW) {
    switch (modeFlag) {
      case 0:
        if (millis() - timeKey < 1000) {
          mode0BlinkFlag ? mode0BlinkFlag = 0 : mode0BlinkFlag = 1;
          Serial.println("double click");
          Serial.println(mode0BlinkFlag);
        }
        timeKey = millis();
        setColor(0, 0, 0);
        Serial.println("DEBUG0");
        while (digitalRead(KEY) == LOW) {
          Serial.println("DEBUG1");
          if (millis() - timeKey > 1000) {
            modeFlag ++;
            break;
          }
        }
        mode0ColorFlag > mode0Num ? mode0ColorFlag = 0 : mode0ColorFlag ++ ;
        break;

        case 1:
        setColor(0, 0, 0);
        while (digitalRead(KEY) == LOW) {
          if (millis() - timeKey > 1000) {
            modeFlag ++;
            break;
          }
        }
        break;

        case 2:
        setColor(0, 0, 0);
        while (digitalRead(KEY) == LOW) {
          if (millis() - timeKey > 1000) {
            modeFlag ++;
            break;
          }
        }
        mode2Speed == 2 ? mode2Speed = 5 : mode2Speed = 2;
        break;
    
      default:
        break;
    }
  }
}

/**
 * r: 8bit 红色
 * g: 8bit 绿色
 * b: 8bit 蓝色
 */
void setColor(u8 r, u8 g, u8 b) {
  analogWrite(R, map(r, 0, 255, 0, 1023));
  analogWrite(G, map(g, 0, 255, 0, 1023));
  analogWrite(B, map(b, 0, 255, 0, 1023));
}

void mode0() {
  static u8 buff;

  if (buff != mode0ColorFlag) {
    buff = mode0ColorFlag;
    Serial.println("mode0");
    Serial.println(mode0ColorFlag);
    delay(500);
    setColor(colorGroup0[mode1ColorFlag][0], colorGroup0[mode1ColorFlag][1], colorGroup0[mode1ColorFlag][2]);
  }
  if (mode0BlinkFlag) {
    Serial.println("DEBUG2");
    setColor(0, 0, 0);
    delay(mode0Speed);
    setColor(colorGroup0[mode1ColorFlag][0], colorGroup0[mode1ColorFlag][1], colorGroup0[mode1ColorFlag][2]);
    delay(mode0Speed);
    Serial.println("DEBUG3");
  }
  
}

void mode1() {
  Serial.println("mode1");
  setColor(colorGroup1[mode1ColorFlag][0], colorGroup1[mode1ColorFlag][1], colorGroup1[mode1ColorFlag][2]);
  u16 delayTime = (u16) colorGroup1[mode1ColorFlag][3] << 8;
  delayTime += (u16) colorGroup1[mode1ColorFlag][4];
  delay(delayTime);
  mode1ColorFlag > mode1Num ? mode1ColorFlag = 0 : mode1ColorFlag ++;
}

void mode2() {
  Serial.println("mode2");
  // u8 sinR, sinG, sinB;
  // sinR = map(sin((double) (mode2ColorFlag / 100) * 3.14) * 100, -100, 100, 0, 255);
  // sinR = map(sin((double) (mode2ColorFlag / 100) * 3.14 + 3.14/3) * 100, -100, 100, 0, 255);
  // sinR = map(sin((double) (mode2ColorFlag / 100) * 3.14 + 3.14*2/3) * 100, -100, 100, 0, 255);

  // setColor(sinR, sinG, sinB);

  // delay(40);

  // mode2ColorFlag > 100 ? mode2ColorFlag = 0 : mode2ColorFlag += mode2Speed;
  
  
  for (int i = 0; i < 64; i++)
  {
    analogWrite(G, map(i, 0, 63, 1023, 0));
    delay(mode2Speed);
  }
  for (int i = 64; i > 0; i--)
  {
    analogWrite(R, map(i - 1, 0, 63, 1023, 0));
    delay(mode2Speed);
  }
  for (int i = 0; i < 64; i++)
  {
    analogWrite(B, map(i, 0, 63, 1023, 0));
    delay(mode2Speed);
  }
  for (int i = 64; i > 0; i--)
  {
    analogWrite(G, map(i - 1, 0, 63, 1023, 0));
    delay(mode2Speed);
  }
  for (int i = 0; i < 64; i++)
  {
    analogWrite(R, map(i, 0, 63, 1023, 0));
    delay(mode2Speed);
  }
  for (int i = 64; i > 0; i--)
  {
    analogWrite(B, map(i - 1, 0, 63, 1023, 0));
    delay(mode2Speed);
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
    if (buff =='\0') {
      return str;
    }
    str += buff;
  }
  return "ERROR";
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
//void getRGBValues() {
//  u8 buff[64];
//  for(u8 i = 0; i < 64; i++) {
//    buff[i] = EEPROM.read(0x20+i);
//  }
//  mode0Num = buff[63];
//  for(u8 i = 0; i < mode0Num; i++) {
//    colorGroup0[i][0] = buff[i*3];
//    colorGroup0[i][1] = buff[i*3+1];
//    colorGroup0[i][2] = buff[i*3+2];
//  }
//  for(u8 i = 0; i < 128; i++) {
//    buff[i] = EEPROM.read(0x60+i);
//  }
//  mode1Num = buff[127];
//  for(u8 i = 0; i < mode1Num; i++) {
//    colorGroup1[i][0] = buff[i*3];
//    colorGroup1[i][1] = buff[i*3+1];
//    colorGroup1[i][2] = buff[i*3+2];
//  }
//}

void settingsInit() {
  File f = SPIFFS.open("/settings.txt", "r");
  if (!f) {
    Serial.println("Open file failed");
  }
  else {
    String data = f.readString();
    Serial.println(data);
    DeserializationError error = deserializeJson(jsonBuffer, data);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    JsonObject root = jsonBuffer.as<JsonObject>();
    String strBuffer = root["color0Num"];
    Serial.print("color0Num : ");
    Serial.println(strBuffer.toInt());
    String strBuffer1 = root["color0Data"]["0"]["r"];
    Serial.println(strBuffer1.toInt());
  }
  f.close();
}
