// HOST-only tests. No emulation of task concurrency, TFT pixels, ADC, or flash.
#include <random>
#include <cstdio>
#include <cstdlib>
#include "Arduino.h"
#include "SettingsApp.h"
#include "DisplayUI.h"
#include "../DrumMIDI_ST7789/DrumMIDI_ST7789.ino"
static unsigned assertions=0;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}++assertions;}while(0)
using namespace drum_settings;
using namespace drum_ui;

static void testSerialization() {
  const Config initial=current;
  uint8_t blob[BLOB_SIZE]{};CHECK(encode(initial,blob,sizeof(blob)));
  Config out{};CHECK(decode(blob,sizeof(blob),out));CHECK(equal(initial,out));
  CHECK(crc32(reinterpret_cast<const uint8_t*>("123456789"),9)==0xcbf43926u);
  for(size_t byte=0;byte<BLOB_SIZE;++byte) for(unsigned bit=0;bit<8;++bit) {
    auto old=blob[byte];blob[byte]^=1u<<bit;out=initial;
    CHECK(!decode(blob,sizeof(blob),out));CHECK(equal(initial,out));blob[byte]=old;
  }
  for(size_t n=0;n<BLOB_SIZE;++n) CHECK(!decode(blob,n,out));
  CHECK(!decode(nullptr,sizeof(blob),out));
  // Even a valid CRC cannot legitimize an invalid setting.
  blob[8+8*10]=10;blob[8+8*10+1]=10;
  const auto crc=crc32(blob,BLOB_SIZE-4);
  for(unsigned i=0;i<4;++i) blob[BLOB_SIZE-4+i]=static_cast<uint8_t>(crc>>(8*i));
  CHECK(!decode(blob,sizeof(blob),out));
}
static void testValidation() {
  const Config initial=current;const char *error=nullptr;
  CHECK(validate(initial,error));
  for(size_t pad=0;pad<PAD_COUNT;++pad) for(size_t p=0;p<PAD_SPECS[pad].size;++p) {
    for(int value=-1;value<=256;++value) {
      Config trial=initial;const bool ok=setPad(trial,pad,p,value,error);
      if(ok) {
        CHECK(validate(trial,error));
        const auto &spec=params(pad)[p];CHECK(value>=spec.low && value<=spec.high);
        CHECK(trial.pads[pad][spec.offset]==value);
        Config expected=initial;expected.pads[pad][spec.offset]=value;
        CHECK(equal(expected,trial));
      } else CHECK(equal(initial,trial));
    }
  }
  Config trial=initial;
  CHECK(!setPad(trial,8,0,10,error)); // Sensitivity == threshold
  CHECK(!setPad(trial,5,2,52,error)); // Pedal start == end
  CHECK(!setPad(trial,6,4,5,error)); // Crash edge == cup
  CHECK(!setPad(trial,99,0,5,error));
  CHECK(!setPad(trial,8,99,5,error));
  CHECK(setPad(trial,8,5,0,error));CHECK(trial.pads[8][4]==0); // Note 0 is valid MIDI.
  CHECK(!setOutput(trial,OutputParam::Velocity,2,error));
  CHECK(!setOutput(trial,OutputParam::Trim,101,error));
  CHECK(!setOutput(trial,static_cast<OutputParam>(99),0,error));
}
static void testButtons() {
  DebouncedButton s(true);s.begin(false,0);
  CHECK(s.poll(true,10,Key::Select)==Key::None);
  CHECK(s.poll(false,20,Key::Select)==Key::None); // contact bounce
  CHECK(s.poll(true,30,Key::Select)==Key::None);
  CHECK(s.poll(true,60,Key::Select)==Key::None); // stable pressed, no short event yet
  CHECK(s.poll(false,100,Key::Select)==Key::None);
  CHECK(s.poll(false,130,Key::Select)==Key::Select);
  CHECK(s.poll(false,140,Key::Select)==Key::None);
  s.begin(false,0);s.poll(true,10,Key::Select);s.poll(true,40,Key::Select);
  CHECK(s.poll(true,840,Key::Select)==Key::Back);
  CHECK(s.poll(true,900,Key::Select)==Key::None);
  s.poll(false,950,Key::Select);CHECK(s.poll(false,980,Key::Select)==Key::None);
  s.begin(true,0);CHECK(s.poll(true,2000,Key::Select)==Key::None);
  s.poll(false,2010,Key::Select);CHECK(s.poll(false,2040,Key::Select)==Key::None);
  DebouncedButton up;up.begin(false,0);
  up.poll(true,10,Key::Up);CHECK(up.poll(true,40,Key::Up)==Key::Up);
  CHECK(up.poll(true,489,Key::Up)==Key::None);
  CHECK(up.poll(true,490,Key::Up)==Key::Up);
  CHECK(up.poll(true,610,Key::Up)==Key::Up);
  // A millis rollover does not cause an immediate long press or missed debounce.
  s.begin(false,0xfffffff0u);s.poll(true,0xfffffff5u,Key::Select);
  CHECK(s.poll(true,0x20u,Key::Select)==Key::None);
  s.poll(false,0x40u,Key::Select);CHECK(s.poll(false,0x60u,Key::Select)==Key::Select);
}
static void testMenuAndSettings() {
  MenuController m;Command c{};auto snap=snapshot();
  auto key=[&](Key k){return m.input(k,snap,c);};
  CHECK(!key(Key::Select));CHECK(m.page()==Page::Main);
  CHECK(!key(Key::Select));CHECK(m.page()==Page::Pads);
  CHECK(!key(Key::Select));CHECK(m.page()==Page::Pad && m.selectedPad()==8);
  CHECK(!key(Key::Select));CHECK(m.editing());
  CHECK(!key(Key::Down));CHECK(m.draft()==99);CHECK(KICK[0]==100);
  CHECK(key(Key::Select));CHECK(m.pending());CHECK(KICK[0]==100);
  CHECK(xQueueSend(commandQueue,&c,0)==pdTRUE);serviceBeforeScan();
  CHECK(KICK[0]==99 && current.pads[8][0]==99);
  CHECK(fakeNvsWrites==0);snap=snapshot();CHECK(snap.dirty);
  m.acknowledge(snap);CHECK(!m.pending());
  CHECK(!key(Key::Select));CHECK(m.editing());CHECK(!key(Key::Down));
  CHECK(!key(Key::Back));CHECK(!m.editing());CHECK(KICK[0]==99); // cancel draft
  CHECK(!key(Key::Back));CHECK(m.page()==Page::Pads);
  CHECK(!key(Key::Back));CHECK(m.page()==Page::Main);
  // Changing a note changes USB MIDI, not the audio slot identifier.
  process({CommandType::SetPad,8,5,36,500});CHECK(KICK[4]==36);
  process({CommandType::SetPad,8,4,4,501});CHECK(kick.curve==4);
  MIDI.messages.clear();drum_runtime::eventQueue->items.clear();
  kick.hit=true;kick.velocity=90;loop();kick.hit=false;
  CHECK(MIDI.messages.size()==2);
  CHECK(MIDI.messages[0][0]==0x90 && MIDI.messages[0][1]==36 && MIDI.messages[0][3]==10);
  drum_runtime::AudioEvent audio{};
  CHECK(xQueueReceive(drum_runtime::eventQueue,&audio,0)==pdTRUE);
  CHECK(audio.id==static_cast<uint8_t>(drum_engine::SampleId::Kick));
  // Master trim and output mute affect audio only; no additional MIDI messages.
  const auto messageCount=MIDI.messages.size();
  process({CommandType::SetOutput,0,2,50,502});CHECK(drum_runtime::masterTargetQ15.load()==4096);
  process({CommandType::SetOutput,0,0,0,503});CHECK(drum_runtime::masterTargetQ15.load()==0);
  process({CommandType::SetOutput,0,1,1,504});CHECK(drum_runtime::audioVelocityEnabled.load());
  CHECK(MIDI.messages.size()==messageCount);
  // Busy save: no flash write while hits are recent, test pending or audio active.
  process({CommandType::Save,0,0,0,505});CHECK(fakeNvsWrites==0);
  fakeNowMs+=1000;drum_runtime::activeVoices.store(1);
  process({CommandType::Save,0,0,0,506});CHECK(fakeNvsWrites==0);
  drum_runtime::activeVoices.store(0);drum_runtime::testRemaining=1;
  process({CommandType::Save,0,0,0,507});CHECK(fakeNvsWrites==0);
  drum_runtime::testRemaining=0;
  process({CommandType::Save,0,0,0,508});CHECK(fakeNvsWrites==1);CHECK(!snapshot().dirty);
  const auto saved=current;
  process({CommandType::Defaults,0,0,0,509});CHECK(KICK[4]==35 && KICK[0]==100 && kick.curve==1);
  CHECK(current.masterTrim==100 && current.audioVelocity==0 && current.localAudio==1);
  CHECK(snapshot().dirty && fakeNvsWrites==1);
  process({CommandType::Reload,0,0,0,510});CHECK(equal(current,saved));CHECK(!snapshot().dirty);
  const Config factory=original;
  for(size_t p=0;p<PAD_COUNT;++p) memcpy(bindings[p].values,factory.pads[p],PAD_SPECS[p].size);
  begin(padBindings); // reset global pad arrays first, as a real reboot does.
  CHECK(equal(original,factory));
  CHECK(equal(current,saved));
  // Corrupt data is never partially applied by manual reload.
  fakeNvs[12]^=1;process({CommandType::Reload,0,0,0,511});CHECK(equal(current,saved));
  fakeNvs[12]^=1;
  process({CommandType::SetPad,8,5,37,512});CHECK(KICK[4]==37);
  fakeNvsFailure=true;process({CommandType::Save,0,0,0,513});
  CHECK(snapshot().dirty && fakeNvsWrites==1 && KICK[4]==37);fakeNvsFailure=false;
  Config reread{};CHECK(readSaved(reread));CHECK(equal(reread,saved));
}
static void testRendererNavigation() {
  Snapshot snap=snapshot();snap.commandSerial=0;
  std::mt19937 random(20261011);
  for(unsigned rotation: {0u,3u}) {
    display.init(240,280);display.setRotation(rotation);
    GFXcanvas16 stripe(display.width(),32);Renderer renderer(display,stripe);
    const unsigned visible=(display.height()-48-42)/28;
    MenuController menu;
    for(unsigned n=0;n<10000;++n) {
      Command cmd{};
      Key k=static_cast<Key>(1+random()%4);
      if(menu.input(k,snap,cmd)) {
        if(cmd.type==CommandType::SetPad) {const char *e=nullptr;setPad(snap.config,cmd.pad,cmd.parameter,cmd.value,e);}
        if(cmd.type==CommandType::SetOutput) {const char *e=nullptr;setOutput(snap.config,static_cast<OutputParam>(cmd.parameter),cmd.value,e);}
        snap.commandSerial=cmd.serial;menu.acknowledge(snap);
      }
      auto view=menu.view(snap,visible);
      CHECK(view.count<=visible);
      renderer.draw(view,visible);
      const unsigned before=display.blits;renderer.draw(view,visible);CHECK(display.blits==before);
    }
  }
}
int main() {
  setup();CHECK(ui_config::validPins());CHECK(drum_runtime::ready);
  CHECK(KICK[4]==35 && SNARE[2]==30 && SNARE[3]==10 && HIHAT_PEDAL[0]==75);
  testSerialization();testValidation();testButtons();testMenuAndSettings();
  testRendererNavigation();
  fakeNowMs+=1000;drum_runtime::testRemaining=0;
  drum_runtime::activeVoices.store(2);drum_runtime::audioFault.store(true);
  CHECK(quietForStorage()); // A dead audio task must not permanently block SAVE.
  drum_runtime::audioFault.store(false);drum_runtime::activeVoices.store(0);
  // Mixer setter changes new-voice velocity, and does not mutate active voices.
  const uint8_t pcm[]={0x10,0x27,0x10,0x27,0x10,0x27,0x10,0x27};
  drum_engine::Sample sample{};sample.pcm=pcm;sample.frames=4;sample.rate=44100;sample.channels=1;
  drum_engine::Mixer<4> mix;mix.begin(44100,true,false,32767);int16_t out[2];
  mix.trigger(sample,1,drum_engine::Group::None);mix.render(out,1);const int originalLevel=out[0];
  mix.setUseVelocity(true);mix.render(out,1);CHECK(out[0]==originalLevel);
  mix.stop();mix.trigger(sample,1,drum_engine::Group::None);mix.render(out,1);
  CHECK(out[0]>0 && out[0]<originalLevel/100);
  drum_ui::destroyBridge();vQueueDelete(drum_runtime::eventQueue);drum_runtime::eventQueue=nullptr;
  std::printf("Settings/menu HOST test: %u assertions PASS\n",assertions);
  std::puts("All parameter byte values, CRC bit corruptions, debounce/long press/rollover PASS");
  std::puts("Draft/apply/cancel, runtime note+curve, output trim/mute/velocity, save/reload/failure PASS");
  std::puts("20,000 randomized menu events across 240x280 and 280x240; dirty-row geometry PASS");
  std::puts("Graphics are mocked: no real font rasterization, screen appearance, or SPI timing tested.");
}
