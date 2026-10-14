#pragma once
#include "Arduino.h"
// Geometry/call-contract mock only. No font data, glyph rasterization, or SPI.
struct GFXfont {};
static const GFXfont FreeSans9pt7b{},FreeSansBold9pt7b{};
class Adafruit_GFX {
 public:
  Adafruit_GFX(int w,int h):w_(w),h_(h){}
  virtual ~Adafruit_GFX()=default;
  int16_t width() const {return w_;} int16_t height() const {return h_;}
  void setTextWrap(bool) {}
  void setFont(const GFXfont *p) {font_=p;}
  void setTextColor(uint16_t) {}
  void getTextBounds(const char *t,int16_t x,int16_t y,int16_t *bx,int16_t *by,uint16_t *w,uint16_t *h) {
    *bx=x;*by=y;*w=strlen(t)*(font_?10:6);*h=font_?18:8;
  }
  void setCursor(int16_t x,int16_t y) {x_=x;y_=y;}
  void print(const char *t) {assert(x_>=0 && x_+static_cast<int>(strlen(t))*(font_?10:6)<=w_);}
  void fillScreen(uint16_t) {}
  void fillRoundRect(int16_t x,int16_t y,int16_t w,int16_t h,int16_t,uint16_t) {
    assert(x>=0 && y>=0 && x+w<=w_ && y+h<=h_);
  }
 protected:int w_,h_,x_=0,y_=0;const GFXfont *font_=nullptr;
};
class GFXcanvas16:public Adafruit_GFX {
 public:
  GFXcanvas16(uint16_t w,uint16_t h):Adafruit_GFX(w,h),data_(w*h){}
  uint16_t *getBuffer() {return data_.data();}
 private:std::vector<uint16_t> data_;
};
