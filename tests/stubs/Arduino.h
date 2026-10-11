#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <array>
#include <deque>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <stdexcept>
using byte=uint8_t;
#define PROGMEM
#define INPUT 0
#define ADC_11db 3
#define CONFIG_IDF_TARGET_ESP32S3 1
#define ARDUINO_USB_MODE 0
#define CONFIG_TINYUSB_MIDI_ENABLED 1
#define SET_USB_MIDI_DEVICE_NAME(name)
static uint32_t fakeNowMs=0;
static uint16_t fakePot=4095;
inline uint32_t millis(){return fakeNowMs;}
inline uint32_t micros(){return fakeNowMs*1000;}
inline void delay(uint32_t ms){fakeNowMs+=ms;}
inline void pinMode(int,int){}
inline uint16_t analogRead(int pin){assert(pin==15);return fakePot;}
inline void analogSetPinAttenuation(int pin,int){assert(pin==15);}
struct FakeSerial {
  void begin(unsigned){}
  int available(){return 0;}
  int read(){return -1;}
  void println(const char*){}
  int printf(const char*,...) __attribute__((format(printf,2,3))) {return 0;}
};
static FakeSerial Serial;

#define INPUT_PULLUP 2
#define LOW 0
#define HIGH 1
static int fakeButtonPins[49] = {};
inline int digitalRead(int p) { return fakeButtonPins[p] ? LOW : HIGH; }
