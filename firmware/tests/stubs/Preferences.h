#pragma once
#include "Arduino.h"
// Deliberately only a host RAM substitute. Does NOT test ESP32 flash behavior.
static std::vector<uint8_t> fakeNvs;
static unsigned fakeNvsWrites=0;
static bool fakeNvsFailure=false;
class Preferences {
 public:
  bool begin(const char*,bool readOnly=false) {
    readOnly_=readOnly;return !fakeNvsFailure && (!readOnly || !fakeNvs.empty());
  }
  void end() {}
  size_t getBytesLength(const char*) {return fakeNvs.size();}
  size_t getBytes(const char*,void *p,size_t size) {
    if(size<fakeNvs.size()) return 0;
    memcpy(p,fakeNvs.data(),fakeNvs.size());return fakeNvs.size();
  }
  size_t putBytes(const char*,const void *p,size_t size) {
    if(readOnly_ || fakeNvsFailure) return 0;
    ++fakeNvsWrites;
    const auto *b=static_cast<const uint8_t*>(p);fakeNvs.assign(b,b+size);return size;
  }
 private:bool readOnly_=false;
};
