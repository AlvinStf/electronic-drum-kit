#pragma once
#define FSPI 1
class SPIClass {
 public:
  explicit SPIClass(int) {}
  void begin(int sck,int miso,int mosi,int cs) {
    assert(sck==12 && miso==-1 && mosi==11 && cs==39);
  }
};
