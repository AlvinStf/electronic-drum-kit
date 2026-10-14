#pragma once
#include "Arduino.h"
struct USBMIDI {
  std::vector<std::array<int,4>> messages;
  void begin(){}
  void noteOn(uint8_t n,uint8_t v,uint8_t c){messages.push_back({0x90,n,v,c});}
  void noteOff(uint8_t n,uint8_t v,uint8_t c){messages.push_back({0x80,n,v,c});}
  void controlChange(uint8_t n,uint8_t v,uint8_t c){messages.push_back({0xB0,n,v,c});}
  void polyPressure(uint8_t n,uint8_t v,uint8_t c){messages.push_back({0xA0,n,v,c});}
};
