#pragma once
#include "Adafruit_GFX.h"
#include "SPI.h"
#define ST77XX_BLACK 0
#define ST77XX_WHITE 65535
#define ST77XX_CYAN 2047
#define ST77XX_YELLOW 65504
class Adafruit_ST7789:public Adafruit_GFX {
 public:
  unsigned blits=0;
  Adafruit_ST7789(SPIClass*,int,int,int):Adafruit_GFX(240,280){}
  void init(uint16_t w,uint16_t h) {w_=w;h_=h;}
  void setRotation(uint8_t r) {if(r&1){w_=280;h_=240;}else{w_=240;h_=280;}}
  void setSPISpeed(uint32_t) {}
  void drawRGBBitmap(int16_t x,int16_t y,uint16_t*,int16_t w,int16_t h) {
    assert(x>=0 && y>=0 && x+w<=w_ && y+h<=h_);++blits;
  }
};
