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

#define GFX_BL   -1 // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define PIN_RST  8
#define PIN_DC   16
#define PIN_CS   11
#define PIN_SCK  14
#define PIN_MOSI 13
#define PIN_MISO 12

#define JPEG_FILENAME "/eduboard1_arduino.jpeg"

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, PIN_MISO);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
//Arduino_GFX *gfx = new Arduino_ILI9488(bus, PIN_RST, 3 /* rotation */, false /* IPS */);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST /* RST */, 1 /* rotation */, false /* IPS */);

FocalTech_Class touch;

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
  Serial.println("\nStart Touch:\n");
  Serial.printf("Touch Init: %i\n", touch.begin(Wire, FOCALTECH_SLAVE_ADDRESS));
  Serial.printf("Touch MonitorTime: %i", touch.getMonitorTime());
  touch.setPowerMode(FOCALTECH_PMODE_ACTIVE);
  touch.disableINT();

  Serial.println("End Setup\n");



  delay(5000);
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
  uint16_t x,y;
  touch.getPoint(x,y);
  uint8_t touched = touch.getTouched();  
  if(touched > 0) {    
    Serial.printf("Touched:%i\n>x:%i\n>y:%i\n", touched, x,y);
  } else {
    Serial.printf("Touched:%i\n", touched);
  }
  delay(10);
}
