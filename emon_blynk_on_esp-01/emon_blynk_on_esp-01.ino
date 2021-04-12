#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <TimeLib.h>
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//#define BLYNK_PRINT Serial
#define BLYNK_HEARTBEAT 120
//#define BLYNK_MSG_LIMIT 40

char auth[] = "9637bb3a8a754461844e4efe20234fc8";

BlynkTimer timer;
WidgetRTC rtc;
WidgetLED errorled(V19);

ESP8266WiFiMulti wifiMulti;
IPAddress local_IP(192, 168, 1, 155);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 167);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

int datalinenumber;
int v1;
int v2;
int v3;
float a1;
float a2;
float a3;
float pf1;
float pf2;
float pf3;
float tp1;
float tp2;
float tp3;
float e1;
float e2;
float e3;
int category;
int t;
int error;
int values[6];
int valueindex;
bool datapresent;

BLYNK_CONNECTED() {
  // Synchronize time on connection
  rtc.begin();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  wifiMulti.addAP("WAW20", "219adeltawab");
  wifiMulti.addAP("WAW20_HK1", "219adeltawab");
  wifiMulti.addAP("WAW20_HK2", "219adeltawab");
  wifiMulti.addAP("WAW20_HK3", "219adeltawab");
  wifiMulti.addAP("HK SG Note 9", "07081989");
  wifiMulti.run();
  WiFi.hostname("HKEmon");
  MDNS.begin("HKEmon");
  Blynk.config(auth);
  timer.setInterval(450, emongetdatafromarduino);
  timer.setInterval(10000, emonsendtowifi);
}

void loop()
{
  wifiMulti.run();
  Blynk.run();
  timer.run();
}

void parsevalues(char* data) {
  char delimiter[] = ",\n";
  char* ptr = strtok(data, delimiter);
  while (ptr != NULL) {
    values[valueindex] = atoi(ptr);
    if (valueindex == 5) {
      valueindex = 0;
      break;
    }
    ptr = strtok(NULL, delimiter);
    valueindex++;
  }
  datalinenumber = values[0];
  switch (datalinenumber) {
    case 1:
      v1 = values[1];
      a1 = (float)values[2] / 10;
      pf1 = (float)values[3] / 100;
      tp1 = values[4];
      e1 = (float)values[5] / 100;
      break;
    case 2:
      v2 = values[1];
      a2 = (float)values[2] / 10;
      pf2 = (float)values[3] / 100;
      tp2 = values[4];
      e2 = (float)values[5] / 100;
      break;
    case 3:
      v3 = values[1];
      a3 = (float)values[2] / 10;
      pf3 = (float)values[3] / 100;
      tp3 = values[4];
      e3 = (float)values[5] / 100;
      break;
    case 4:
      t = values[1];
      error = values[2];
      category = values[3];
      break;
  }
}

void emongetdatafromarduino() {
  if (Serial.available()) {
    if (Serial.read() == '>') {
      datapresent = true;
      char data[32] = "";  // example = >2,225,42,38,56552,616521
      Serial.readBytes(data, 31);
      parsevalues(data);
      //emonsendtoserial();
    }
  }
}

void emonsendtowifi() {
  if (Blynk.connected()) {
    Serial.print(F("<"));
    Serial.print(now());
    Serial.print(F(":"));
    Serial.print(WiFi.localIP());
    Serial.println(F(":1>"));
  }
  else {
    Serial.print(F("<"));
    Serial.print(now());
    Serial.print(F(":"));
    Serial.print(WiFi.localIP());
    Serial.println(F(":3>"));
  }
  if (datapresent) {
    Blynk.virtualWrite(V0, t);
    yield();

    Blynk.virtualWrite(V1, v1);
    yield();
    Blynk.virtualWrite(V2, a1);
    yield();
    Blynk.virtualWrite(V3, pf1);
    yield();
    Blynk.virtualWrite(V4, tp1);
    yield();

    Blynk.virtualWrite(V5, v2);
    yield();
    Blynk.virtualWrite(V6, a2);
    yield();
    Blynk.virtualWrite(V7, pf2);
    yield();
    Blynk.virtualWrite(V8, tp2);
    yield();

    Blynk.virtualWrite(V9, v3);
    yield();
    Blynk.virtualWrite(V10, a3);
    yield();
    Blynk.virtualWrite(V11, pf3);
    yield();
    Blynk.virtualWrite(V12, tp3);
    yield();

    Blynk.virtualWrite(V13, tp1 + tp2 + tp3);
    yield();

    Blynk.virtualWrite(V14, e1);
    yield();
    Blynk.virtualWrite(V15, e2);
    yield();
    Blynk.virtualWrite(V16, e3);
    yield();

    Blynk.virtualWrite(V17, e1 + e2 + e3);
    yield();

    Blynk.virtualWrite(V18, category);
    yield();

    if (error == 1) {
      errorled.on();
    }
    else {
      errorled.off();
    }
    yield();

    Serial.print(F("<"));
    Serial.print(now());
    Serial.print(F(":"));
    Serial.print(WiFi.localIP());
    Serial.println(F(":2>"));
    datapresent = false;
  }
}

void emonsendtoserial() {
  Serial.print("L");
  Serial.print(datalinenumber);
  Serial.print('\t');
  Serial.print(values[1]);
  Serial.print('\t');
  Serial.print((float)values[2] / 10, 1);
  Serial.print('\t');
  Serial.print((float)values[3] / 100, 2);
  Serial.print('\t');
  Serial.print((float)values[4] / 10, 1);
  Serial.print('\t');
  Serial.println(values[5]);
}
