#pragma once
#include "Arduino.h"
struct StopAudioTest{};
namespace audio_tools {
enum RxTxMode{TX_MODE=1};
enum class AudioToolsLogLevel{Error};
struct Logger {void begin(FakeSerial&,AudioToolsLogLevel){}};
static Logger AudioToolsLogger;
struct Config {
  uint32_t sample_rate=0; int channels=0,bits_per_sample=0;
  int pin_bck=-1,pin_ws=-1,pin_data=-1,pin_mck=-1;
  int buffer_size=0,buffer_count=0;
};
static int writesUntilStop=-1;
static size_t maxWrite=4096;
static bool beginOkay=true;
struct I2SStream {
  Config config{};
  std::vector<uint8_t> output;
  Config defaultConfig(RxTxMode){return {};}
  bool begin(Config c){config=c;return beginOkay;}
  void end(){}
  size_t write(const uint8_t* p,size_t n) {
    if(writesUntilStop==0)throw StopAudioTest{};
    if(writesUntilStop>0)--writesUntilStop;
    const size_t sent=std::min(n,maxWrite);
    output.insert(output.end(),p,p+sent); return sent;
  }
};
}
