#include <Arduino.h>
#include <WIRE.h>
#include <Arduino_GFX_Library.h>
//#include <FFat.h>
//#include <LittleFS.h>
#include <SPIFFS.h>
//#include <SD.h>
//#include <SD_MMC.h>
#include "JpegFunc.h"
#include "focaltech.h"
#include "dac5571.h"
#include "stk8baxx.h"
#include "RTClib.h"

#define GFX_BL   -1 // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define PIN_RST  8
#define PIN_DC   16
#define PIN_CS   11
#define PIN_SCK  14
#define PIN_MOSI 13
#define PIN_MISO 12

#define DAC_ADDR_1  0x4C
#define DAC_ADDR_2  0x4D

#define JPEG_FILENAME "/eduboard1_arduino.jpeg"

const uint16_t sineLookupTable[] = {
0x80, 0x86, 0x8c, 0x92, 0x98, 0x9e, 0xa5, 0xaa,
0xb0, 0xb6, 0xbc, 0xc1, 0xc6, 0xcb, 0xd0, 0xd5,
0xda, 0xde, 0xe2, 0xe6, 0xea, 0xed, 0xf0, 0xf3,
0xf5, 0xf8, 0xfa, 0xfb, 0xfd, 0xfe, 0xfe, 0xff,
0xff, 0xff, 0xfe, 0xfe, 0xfd, 0xfb, 0xfa, 0xf8,
0xf5, 0xf3, 0xf0, 0xed, 0xea, 0xe6, 0xe2, 0xde,
0xda, 0xd5, 0xd0, 0xcb, 0xc6, 0xc1, 0xbc, 0xb6,
0xb0, 0xaa, 0xa5, 0x9e, 0x98, 0x92, 0x8c, 0x86,
0x80, 0x79, 0x73, 0x6d, 0x67, 0x61, 0x5a, 0x55,
0x4f, 0x49, 0x43, 0x3e, 0x39, 0x34, 0x2f, 0x2a,
0x25, 0x21, 0x1d, 0x19, 0x15, 0x12, 0x0f, 0x0c,
0x0a, 0x07, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00,
0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x05, 0x07,
0x0a, 0x0c, 0x0f, 0x12, 0x15, 0x19, 0x1d, 0x21,
0x25, 0x2a, 0x2f, 0x34, 0x39, 0x3e, 0x43, 0x49,
0x4f, 0x55, 0x5a, 0x61, 0x67, 0x6d, 0x73, 0x79};

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
//Arduino_GFX *gfx = new Arduino_ILI9488(bus, PIN_RST, 3 /* rotation */, false /* IPS */);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST /* RST */, 1 /* rotation */, false /* IPS */);

FocalTech_Class touch;

STK8xxx stk8xxx;

RTC_PCF8563 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

static int jpegDrawCallback(JPEGDRAW *pDraw)
{
  //Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}


void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("JPEG Image Viewer");

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

/* Wio Terminal */
  // if (!FFat.begin())
  //if (!LittleFS.begin())
  if (!SPIFFS.begin())
  // if (!SD.begin(SS))
  // pinMode(10 /* CS */, OUTPUT);
  // digitalWrite(10 /* CS */, HIGH);
  // SD_MMC.setPins(12 /* CLK */, 11 /* CMD/MOSI */, 13 /* D0/MISO */);
  // if (!SD_MMC.begin("/root", true))
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }
  else
  {
    unsigned long start = millis();
    jpegDraw(JPEG_FILENAME, jpegDrawCallback, true /* useBigEndian */,
             0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
    Serial.printf("Time used: %lu\n", millis() - start);
  }
  Wire.begin(2, 1, 400000);
  // Serial.println("\nStart Touch:\n");
  // Serial.printf("Touch Init: %i\n", touch.begin(Wire, FOCALTECH_SLAVE_ADDRESS));
  // Serial.printf("Touch MonitorTime: %i", touch.getMonitorTime());
  // touch.setPowerMode(FOCALTECH_PMODE_ACTIVE);
  // touch.disableINT();

  stk8xxx.STK8xxx_Initialization();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.start();

  Serial.println("End Setup\n");



  delay(5000);
}

uint8_t dacpos1 = 0;
uint8_t dacpos2 = 0;

void getGSensorData(float *X_DataOut, float *Y_DataOut, float *Z_DataOut)
{
    *X_DataOut = 0;
    *Y_DataOut = 0;
    *Z_DataOut = 0;
    stk8xxx.STK8xxx_Getregister_data(X_DataOut, Y_DataOut, Z_DataOut);
}

void rtctest() {
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");

  // calculate a date which is 7 days, 12 hours and 30 seconds into the future
  DateTime future (now + TimeSpan(7,12,30,6));

  Serial.print(" now + 7d + 12h + 30m + 6s: ");
  Serial.print(future.year(), DEC);
  Serial.print('/');
  Serial.print(future.month(), DEC);
  Serial.print('/');
  Serial.print(future.day(), DEC);
  Serial.print(' ');
  Serial.print(future.hour(), DEC);
  Serial.print(':');
  Serial.print(future.minute(), DEC);
  Serial.print(':');
  Serial.print(future.second(), DEC);
  Serial.println();

  Serial.println();
}

void loop() {
  //Serial.printf("Start Loop");
  // int w = gfx->width();
  // int h = gfx->height();

  // unsigned long start = millis();
  // jpegDraw(JPEG_FILENAME, jpegDrawCallback, true /* useBigEndian */, 0, 0, w, h);
  // //Serial.printf("Time used: %lu\n", millis() - start);
  // Serial.printf("Total heap: %d\n", ESP.getHeapSize());
  // Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  // Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
  // delay(1000);
  // uint16_t x,y;
  // touch.getPoint(x,y);
  // uint8_t touched = touch.getTouched();  
  // if(touched > 0) {    
  //   Serial.printf("Touched:%i\n>x:%i\n>y:%i\n", touched, x,y);
  // } else {
  //   //Serial.printf("Touched:%i\n", touched);
  // }
  // dacpos1++;
  // dacpos2++;
  // setVoltage(sineLookupTable[dacpos1 & 0x7F],DAC_ADDR_1);
  // delay(20);  
  // setVoltage(sineLookupTable[dacpos2 & 0x7F],DAC_ADDR_2);
  // float x,y,z;
  // getGSensorData(&x,&y,&z);
  // Serial.printf("X:%f - Y:%f - Z:%f\n", x,y,z);

  rtctest();
  delay(2000);  
}
