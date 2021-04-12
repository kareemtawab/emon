#include <UTFT.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Timer.h"
#include <SPI.h>
#include <TimeLib.h>
#include <FastLED.h>

//#define DEBUG_RF_ON_LCD

#define CATEGORY_COUNT 7
#define STATUSLED LED_BUILTIN
#define NUM_LEDS 1
#define LED_DATA_PIN 21
CRGB leds[NUM_LEDS];

Timer timer;

UTFT myGLCD(R61581, 38, 39, 40, 41); // 480x320 pixels
extern uint8_t Sinclair_S[];
extern uint8_t Sinclair_M[];
extern uint8_t Calibri24x32GR[];
extern uint8_t various_symbols[];
extern uint8_t Retro8x16[];
extern uint8_t GroteskBold16x32[];

RF24 radio(8, 7); // CE, CSN
const byte address[6] = "00001";

int datalinenumber;
int datalineorderforesp;
bool s1;
bool s2;
bool s3;
bool s4;

int v;
int a;
int pf;
int tp;
int e;

int v1;
float a1;
float pf1;
int tp1;
float e1;

int v2;
float a2;
float pf2;
int tp2;
float e2;

int v3;
float a3;
float pf3;
int tp3;
float e3;

int ttp;
float te;

int t;

int blipinterval;
unsigned long previousMillis = 0;
char datatoespinchar[32] = "";
char datafromrfinchar[32] = "";
long unix;
char ip[20];
int error = 1;
int currentcategory;
int categorylimits[CATEGORY_COUNT] = {0, 50, 100, 200, 350, 650, 1000};

void setup() {
  Serial.begin(115200);
  Serial3.begin(115200);
  pinMode(STATUSLED, OUTPUT);
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);
  myGLCD.InitLCD();
  myGLCD.fillScr(0, 0, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(GroteskBold16x32);
  myGLCD.print("Home Energy Monitor", 10, 7);
  myGLCD.print("& Phase Sequence Detector", 10, 40);
  myGLCD.setFont(Sinclair_M);
  myGLCD.print("Kareem A. Tawab", 10, 85);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print("Compiled: 03/2021", 10, 105);
  radio.begin();
  //radio.setCRCLength(RF24_CRC_8);          // Use 8-bit CRC for performance
  //radio.setPayloadSize(32);
  radio.setChannel(108);
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);
  radio.openReadingPipe(0, address);
  radio.startListening();
  timer.every(75, emon_get_data_from_rf);
  timer.every(50, emon_draw_on_tft);
  timer.every(10000, emon_send_to_esp);
  delay(3500);
  myGLCD.fillScr(0, 0, 0);
}

void loop() {
  timer.update();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blipinterval) {
    previousMillis = currentMillis;
    blip();
  }
  for (int i = 1; i <= CATEGORY_COUNT; i++) {
    if (te < categorylimits[i] && te > categorylimits[i - 1]) {
      currentcategory = i;
      break;
    }
    else {
      continue;
    }
  }
}

void blip() {
  if (error == 1) {
    FastLED.setBrightness(255);
    leds[0] = CRGB::Red;
    FastLED.show();
  }
  else if (v1 != 0 && v2 != 0 && v3 != 0) {
    FastLED.setBrightness(255);
    leds[0] = CRGB::White;
    FastLED.show();
    digitalWrite(STATUSLED, HIGH);
    delay(30);
    leds[0] = CRGB::Black;
    FastLED.show();
    digitalWrite(STATUSLED, LOW);
  }
}

void emon_get_data_from_rf() {
  if (radio.available()) {
    radio.read(&datafromrfinchar, sizeof(datafromrfinchar));
    radio.flush_rx();

    if (datafromrfinchar[0] == '>') {
      sscanf(datafromrfinchar, ">%d,%d,%d,%d,%d,%d", &datalinenumber, &v, &a, &pf, &tp, &e);
      switch (datalinenumber) {
        case 1:
          v1 = v;
          a1 = (float)a / 10;
          pf1 = (float)pf / 100;
          tp1 = tp;
          e1 = (float)e / 100;
          s1 = true;
          break;
        case 2:
          v2 = v;
          a2 = (float)a / 10;
          pf2 = (float)pf / 100;
          tp2 = tp;
          e2 = (float)e / 100;
          s2 = true;
          break;
        case 3:
          v3 = v;
          a3 = (float)a / 10;
          pf3 = (float)pf / 100;
          tp3 = tp;
          e3 = (float)e / 100;
          s3 = true;
          break;
        case 4:
          t = v;
          error = a;
          s4 = true;
          break;
      }
    }
    ttp = tp1 + tp2 + tp3;
    te = e1 + e2 + e3;
    blipinterval = ((3600 / ((float)ttp / 1000)) / 1600) * 1000; //in ms, for meters 1600imp/kWh
  }
  else {
    datalinenumber = 0;
    s1 = false;
    s2 = false;
    s3 = false;
    s4 = false;
  }
}

void emon_send_to_esp() {
  if (v1 != 0 && v2 != 0 && v3 != 0) {
    String dataline;
    dataline.reserve(32);
    if (datalineorderforesp < 5) {
      switch (datalineorderforesp) {
        case 1:
          if (v1 != 0) {
            dataline = ">1," + String(v1) + "," + String((int)(a1 * 10)) + "," + String((int)(pf1 * 100)) + "," + String(tp1) + "," + String((long)(e1 * 100));
            Serial3.println(dataline);
          }
          break;
        case 2:
          if (v2 != 0) {
            dataline = ">2," + String(v2) + "," + String((int)(a2 * 10)) + "," + String((int)(pf2 * 100)) + "," + String(tp2) + "," + String((long)(e2 * 100));
            Serial3.println(dataline);
          }
          break;
        case 3:
          if (v3 != 0) {
            dataline = ">3," + String(v3) + "," + String((int)(a3 * 10)) + "," + String((int)(pf3 * 100)) + "," + String(tp3) + "," + String((long)(e3 * 100));
            Serial3.println(dataline);
          }
          break;
        case 4:
          if (t != 0) {
            dataline = ">4," + String(t) + "," + String(error) + "," + String(currentcategory);
            Serial3.println(dataline);
          }
          break;
      }
      Serial3.end();
      Serial3.begin(115200);
      datalineorderforesp++;
    }
    else {
      datalineorderforesp = 1;
    }
  }
}

void emon_draw_on_tft() {
#ifdef DEBUG_RF_ON_LCD
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(0, 0, 479, 30);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(Sinclair_M);
  myGLCD.print(datafromrfinchar, 5, 5);
#endif
#ifndef DEBUG_RF_ON_LCD
  myGLCD.setColor(255, 0, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawRoundRect(6, 5, 473, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.setFont(Sinclair_M);
  myGLCD.print("PHASE L1", 10, 7);
  myGLCD.print("V", 84, 53);
  myGLCD.print("A", 210, 53);
  myGLCD.print("W", 343, 53);
  myGLCD.setFont(Retro8x16);
  myGLCD.printNumF(pf1, 2, 415, 32, '.', 4, ' ');
  myGLCD.print("PF", 450, 32);
  myGLCD.printNumF(e1, 2, 384, 51, '.', 7, ' ');
  myGLCD.print("kWh", 442, 51);
  myGLCD.setFont(Calibri24x32GR);
  myGLCD.printNumI(v1, 10, 26, 3, ' ');
  myGLCD.printNumF(a1, 1, 112, 26, '.', 4, ' ');
  myGLCD.printNumI(tp1, 245, 26, 4, ' ');
  myGLCD.setFont(various_symbols);
  if (s1) {
    myGLCD.print("r", 145, 8);
  }
  else {
    myGLCD.setColor(40, 40, 40);
    myGLCD.print("r", 145, 8);
  }

  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawRoundRect(6, 80, 473, 152);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setFont(Sinclair_M);
  myGLCD.print("PHASE L2", 10, 82);
  myGLCD.print("V", 84, 131);
  myGLCD.print("A", 210, 131);
  myGLCD.print("W", 343, 131);
  myGLCD.setFont(Retro8x16);
  myGLCD.printNumF(pf2, 2, 415, 107, '.', 4, ' ');
  myGLCD.print("PF", 450, 108);
  myGLCD.printNumF(e2, 2, 384, 127, '.', 7, ' ');
  myGLCD.print("kWh", 442, 127);
  myGLCD.setFont(Calibri24x32GR);
  myGLCD.printNumI(v2, 10, 103, 3, ' ');
  myGLCD.printNumF(a2, 1, 112, 103, '.', 4, ' ');
  myGLCD.printNumI(tp2, 245, 103, 4, ' ');
  myGLCD.setFont(various_symbols);
  if (s2) {
    myGLCD.print("r", 145, 83);
  }
  else {
    myGLCD.setColor(40, 40, 40);
    myGLCD.print("r", 145, 83);
  }

  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawRoundRect(6, 156, 473, 227);
  myGLCD.setColor(0, 0, 255);
  myGLCD.setFont(Sinclair_M);
  myGLCD.print("PHASE L3", 10, 158);
  myGLCD.print("V", 84, 206);
  myGLCD.print("A", 210, 206);
  myGLCD.print("W", 343, 206);
  myGLCD.setFont(Retro8x16);
  myGLCD.printNumF(pf3, 2, 415, 182, '.', 4, ' ');
  myGLCD.print("PF", 450, 182);
  myGLCD.printNumF(e3, 2, 384, 202, '.', 7, ' ');
  myGLCD.print("kWh", 442, 202);
  myGLCD.setFont(Calibri24x32GR);
  myGLCD.printNumI(v3, 10, 179, 3, ' ');
  myGLCD.printNumF(a3, 1, 112, 179, '.', 4, ' ');
  myGLCD.printNumI(tp3, 245, 179, 4, ' ');
  myGLCD.setFont(various_symbols);
  if (s3) {
    myGLCD.print("r", 145, 159);
  }
  else {
    myGLCD.setColor(40, 40, 40);
    myGLCD.print("r", 145, 159);
  }

  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(Calibri24x32GR);
  myGLCD.drawRoundRect(6, 236, 157, 288);
  myGLCD.printNumI(ttp, 10, 240, 5, ' ');
  myGLCD.drawRoundRect(246, 236, 473, 288);
  myGLCD.printNumF(te, 2, 249, 240, '.', 7, ' ');
  myGLCD.setFont(Sinclair_M);
  myGLCD.print("W", 137, 268);
  myGLCD.print("kWh", 421, 268);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print("LT", 431, 249);
  myGLCD.setFont(Sinclair_M);
  myGLCD.printNumI(currentcategory, 454, 245);

  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print("PHASE", 162, 240);
  myGLCD.print("SEQUENCE", 162, 250);
  myGLCD.setFont(Sinclair_M);
  if (error == 1) {
    myGLCD.print("ERROR", 162, 263);
  }
  else {
    myGLCD.print("OK   ", 162, 263);
  }

  myGLCD.setColor(40, 40, 40);
  myGLCD.drawRect(0, 295, 479, 296);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(Sinclair_S);
  myGLCD.print(",   :  ", RIGHT, 300);
  myGLCD.printNumI(minute(), 480 - 16, 300, 2, '0');
  myGLCD.printNumI(hour(), 480 - 40, 300, 2, '0');
  myGLCD.printNumI(year(), 480 - 88, 300, 4, '0');
  myGLCD.printNumI(day(), 480 - 112, 300, 2, '0');
  myGLCD.print(monthShortStr(month()), 480 - 144, 300);
  myGLCD.print("T:   C", RIGHT, 310);
  myGLCD.drawRect(467, 311, 468, 312);
  myGLCD.printNumI(t, 480 - 31, 310, 2, ' ');
  myGLCD.print("ESP-01:", 2, 300);
  myGLCD.print("IP:", 2, 310);
  if (Serial3.available()) {
    if (Serial3.read() == '<') {
      unix = Serial3.parseInt();
      setTime(unix);
      int ipint[4];
      ipint[0] = Serial3.parseInt();
      ipint[1] = Serial3.parseInt();
      ipint[2] = Serial3.parseInt();
      ipint[3] = Serial3.parseInt();
      sprintf(ip, "%d.%d.%d.%d", ipint[0], ipint[1], ipint[2], ipint[3]);
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(60, 300, 303, 307);
      myGLCD.fillRect(28, 310, 303, 317);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print(ip, 28, 310);
      int m = Serial3.parseInt();
      switch (m) {
        case 1:
          //connected and idle
          myGLCD.print("CONNECTED, IDLE", 60, 300);
          break;
        case 2:
          //updating blynk servers
          myGLCD.print("UPDATING BLYNK SERVERS...", 60, 300);
          break;
        case 3:
          //disconnected
          myGLCD.print("DISCONNECTED!", 60, 300);
          break;
      }
      Serial3.read();
      Serial3.end();
      Serial3.begin(115200);
    }
  }
#endif
}
