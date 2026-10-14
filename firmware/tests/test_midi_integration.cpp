// This is a HOST simulation with stub hardware APIs, NOT an ESP32 firmware build.
#include <random>
#include <cstdio>
#include <cstdlib>
#include "Arduino.h"
#include "hellodrum.h"
#include "USB.h"
#include "USBMIDI.h"
#include "AudioRuntime.h"
#include "SettingsApp.h"
#include "DisplayUI.h"
namespace baseline {
#include "fixtures/DrumMIDI_original.ino.txt"
}
namespace integrated {
#include "../DrumMIDI_ST7789/DrumMIDI_ST7789.ino"
}
static unsigned tests=0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort(); } ++tests; } while(0)
static HelloDrum *oldPads[]={&baseline::highTom,&baseline::highMidTom,&baseline::floorTom,
  &baseline::snare,&baseline::hihat,&baseline::hihatPedal,&baseline::crash,&baseline::ride,&baseline::kick};
static HelloDrum *newPads[]={&integrated::highTom,&integrated::highMidTom,&integrated::floorTom,
  &integrated::snare,&integrated::hihat,&integrated::hihatPedal,&integrated::crash,&integrated::ride,&integrated::kick};
static std::mt19937 rng(314159);
static void clearFlags(HelloDrum &p) {
  p.hit=p.hitRim=p.hitCup=p.choke=p.openHH=p.closeHH=p.moving=false;
}
static void randomPads() {
  for(unsigned i=0;i<9;++i){
    auto &p=*oldPads[i]; const auto f=rng();
    p.hit=f&1; p.hitRim=f&2; p.hitCup=f&4; p.choke=f&8;
    p.openHH=f&16; p.closeHH=f&32; p.moving=f&64;
    p.velocity=1+(rng()%127); p.pedalCC=(rng()&1)?127:0;
    *newPads[i]=p; p.scans.clear(); newPads[i]->scans.clear();
  }
}
static unsigned compareLoops(){
  baseline::MIDI.messages.clear(); integrated::MIDI.messages.clear();
  baseline::loop(); integrated::loop();
  CHECK(baseline::MIDI.messages==integrated::MIDI.messages);
  for(unsigned i=0;i<9;++i) {
    CHECK(oldPads[i]->scans==newPads[i]->scans);
    oldPads[i]->scans.clear();newPads[i]->scans.clear();
  }
  unsigned noteOns=0;
  for(const auto &e:baseline::MIDI.messages) { CHECK(e[3]==10); noteOns+=e[0]==0x90; }
  return noteOns;
}
int main(){
  baseline::setup(); integrated::setup();
  using namespace drum_runtime;
  CHECK(ready && !audioFault.load() && loadedSamples==1);
  CHECK(i2s.config.pin_bck==16 && i2s.config.pin_ws==17 && i2s.config.pin_data==18);
  CHECK(i2s.config.channels==2 && i2s.config.sample_rate==44100);
  for(unsigned i=0;i<9;++i) {
    CHECK(oldPads[i]->pin1==newPads[i]->pin1 && oldPads[i]->pin2==newPads[i]->pin2);
    CHECK(oldPads[i]->curve==newPads[i]->curve);
  }
  // Supply dummy TEST-ONLY mappings for every zone so all audio routes can be
  // checked. The distributed firmware still has only the user-provided kick.
  for(size_t i=1;i<drum_engine::SAMPLE_COUNT;++i)samples[i]=samples[0];
  for(unsigned n=0;n<4096;++n){
    eventQueue->items.clear(); randomPads(); fakeNowMs+=5;
    const unsigned notes=compareLoops();
    unsigned playEvents=0;
    for(const auto &b:eventQueue->items) {
      AudioEvent e;std::memcpy(&e,b.data(),sizeof(e));
      if(e.type==EventType::Play) { ++playEvents; CHECK(e.id<16 && e.velocity>=1 && e.velocity<=127); }
    }
    CHECK(playEvents==notes);
  }
  // Explicit snare velocity boundary coverage with no other simultaneous pads.
  for(int v=1;v<=127;++v) {
    for(unsigned i=0;i<9;++i){clearFlags(*oldPads[i]);clearFlags(*newPads[i]);}
    baseline::snare.hitRim=integrated::snare.hitRim=true;
    baseline::snare.velocity=integrated::snare.velocity=v;
    eventQueue->items.clear(); compareLoops();
    CHECK(integrated::MIDI.messages[0][1]==(v>60?40:37));
    CHECK(integrated::MIDI.messages[0][2]==(v>60?v:v*2));
  }
  // Same MIDI note (53), distinct local WAV slot IDs.
  for(unsigned i=0;i<9;++i){clearFlags(*oldPads[i]);clearFlags(*newPads[i]);}
  baseline::crash.hitCup=integrated::crash.hitCup=true;
  baseline::ride.hitCup=integrated::ride.hitCup=true;
  eventQueue->items.clear(); compareLoops();
  CHECK(eventQueue->items.size()==2);
  AudioEvent a,b;
  std::memcpy(&a,eventQueue->items[0].data(),sizeof(a));
  std::memcpy(&b,eventQueue->items[1].data(),sizeof(b));
  CHECK(a.id==static_cast<uint8_t>(SampleId::CrashCup) && b.id==static_cast<uint8_t>(SampleId::RideCup));
  // Missing non-kick sounds do not become fake kick sounds or suppress MIDI.
  for(size_t i=1;i<drum_engine::SAMPLE_COUNT;++i)samples[i]=drum_engine::Sample{};
  eventQueue->items.clear(); randomPads(); compareLoops();
  for(const auto &raw:eventQueue->items) {
    AudioEvent e;std::memcpy(&e,raw.data(),sizeof(e));
    CHECK(e.type==EventType::Choke || e.id==static_cast<uint8_t>(SampleId::Kick));
  }
  // A missing closed-HH sample STILL releases open-HH audio when added later.
  eventQueue->items.clear(); play(SampleId::HihatClosed,80);
  CHECK(eventQueue->items.size()==1);
  std::memcpy(&a,eventQueue->items[0].data(),sizeof(a));
  CHECK(a.type==EventType::Choke && a.group==static_cast<uint8_t>(Group::HihatOpen));
  // Audio startup failure, runtime fault, and full queue never stop USB MIDI.
  ready=false; randomPads(); compareLoops(); ready=true;
  audioFault.store(true); randomPads(); compareLoops(); audioFault.store(false);
  eventQueue->items.clear();
  for(unsigned i=0;i<drum_config::EVENT_CAPACITY;++i)CHECK(play(SampleId::Kick,100));
  const auto droppedBefore=droppedEvents;
  baseline::kick.hit=integrated::kick.hit=true;compareLoops();CHECK(droppedEvents>droppedBefore);
  // Pot at min/max changes ONLY the audio gain target, not MIDI messages.
  for(int extreme: {0,4095}) {
    fakePot=extreme;
    for(int i=0;i<200;++i){fakeNowMs+=5;updateVolume();}
    CHECK(masterTargetQ15.load()==(extreme?8192u:0u));
    eventQueue->items.clear();randomPads();compareLoops();
  }
  // Runtime task: service a real queue, render, and retry partial I2S writes.
  eventQueue->items.clear(); mixer.begin(44100,true,false,0);
  masterTargetQ15.store(8192); startedHits.store(0); writeErrors.store(0);
  play(SampleId::Kick,110);
  audio_tools::writesUntilStop=4;audio_tools::maxWrite=64;
  try {audioTask(nullptr);} catch(const StopAudioTest&) {}
  CHECK(startedHits.load()==1 && writeErrors.load()==0 && !audioFault.load());
  CHECK(i2s.output.size()==drum_config::BLOCK_FRAMES*4);
  // Zero-length write is surfaced as a fault; MIDI still executes normally.
  audio_tools::writesUntilStop=-1;audio_tools::maxWrite=0;
  audioTask(nullptr); CHECK(audioFault.load() && writeErrors.load()==1);
  randomPads();compareLoops();
  drum_ui::destroyBridge();
  vQueueDelete(eventQueue); eventQueue=nullptr;
  std::printf("MIDI integration HOST STUB test: 4096 randomized states + all 127 snare velocities\n");
  std::printf("%u assertions PASS; original MIDI messages, scan arguments, and pad pins preserved\n",tests);
  std::puts("Missing WAV / audio fault / full queue / pot endpoint / partial-write cases PASS");
  std::puts("Not a native ESP32 build; stubs do not model ADC, USB enumeration, DMA, or scheduling timing.");
}
