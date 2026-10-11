#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <random>
#include "DrumEngine.h"
#include "MasterVolume.h"
#include "WavData.h"
using namespace drum_engine;
static unsigned tests = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); std::abort(); } ++tests; } while(0)
static void p16(std::vector<uint8_t> &b, size_t p, uint16_t x) { b[p]=x&255; b[p+1]=x>>8; }
static void p32(std::vector<uint8_t> &b, size_t p, uint32_t x) { for (int j=0;j<4;++j)b[p+j]=(x>>(8*j))&255; }
static std::vector<uint8_t> wav(const std::vector<int16_t> &pcm, uint16_t ch=1) {
  std::vector<uint8_t> b(44+pcm.size()*2,0);
  std::memcpy(b.data(),"RIFF",4); p32(b,4,b.size()-8); std::memcpy(b.data()+8,"WAVEfmt ",8);
  p32(b,16,16); p16(b,20,1); p16(b,22,ch); p32(b,24,44100);
  p32(b,28,44100*2*ch); p16(b,32,2*ch); p16(b,34,16);
  std::memcpy(b.data()+36,"data",4); p32(b,40,pcm.size()*2);
  for (size_t i=0;i<pcm.size();++i)p16(b,44+2*i,static_cast<uint16_t>(pcm[i]));
  return b;
}
static Sample parse(const std::vector<uint8_t> &b) {
  Sample s; const char *e=nullptr; CHECK(parseWav(b.data(),b.size(),s,e)); return s;
}
int main() {
  Sample kick; const char *error=nullptr;
  CHECK(parseWav(WavData,sizeof(WavData),kick,error));
  CHECK(kick.frames==26496 && kick.rate==44100 && kick.channels==1);
  CHECK(sizeof(WavData)==53036);
  auto b=wav({1000,-1000,32767,-32768}); auto s=parse(b);
  int16_t out[2048]{};
  Mixer<16> m; m.begin(44100,true,false,8192);
  CHECK(m.trigger(s,127,Group::None)==TriggerResult::Started);
  CHECK(m.render(out,4)==0);
  CHECK(out[0]==249 && out[2]==-249 && out[4]==8191 && out[6]==-8191);
  CHECK(out[0]==out[1] && out[2]==out[3] && m.activeVoices()==0);
  auto stereoBytes=wav({1000,-2000,-3000,4000},2); auto stereo=parse(stereoBytes);
  m.begin(44100,true,false,32767); m.trigger(stereo,127,Group::None); m.render(out,2);
  CHECK(out[0]==998 && out[1]==-1998 && out[2]==-2998 && out[3]==3998);
  // Independent sounds sum, even when their payload happens to be identical.
  m.begin(44100,true,false,8192); m.trigger(s,127,Group::None); m.trigger(stereo,127,Group::None);
  m.render(out,1); CHECK(out[0]==499 && out[1]==-250);
  auto steadyBytes=wav(std::vector<int16_t>(5000,16000)); auto steady=parse(steadyBytes);
  // Retrigger starts immediately rather than waiting for end of sample.
  m.begin(44100,true,false,8192); m.trigger(steady,127,Group::None); m.render(out,64);
  m.trigger(steady,127,Group::None); CHECK(m.activeVoices()==2); m.render(out,1); CHECK(out[0]==7999);
  // Use real WAV: 16 hits spaced 100 ms. Compare peak voice count and clip count.
  m.begin(44100,true,false,8192); unsigned peak=0,clips=0,hits=0;
  const size_t end=15*4410+kick.frames+64;
  for(size_t i=0;i<end;++i) {
    if(i%4410==0 && hits<16) { CHECK(m.trigger(kick,110,Group::None)==TriggerResult::Started); ++hits; }
    peak=std::max<unsigned>(peak,m.activeVoices()); clips+=m.render(out,1);
  }
  CHECK(hits==16 && peak==7 && clips==0 && m.activeVoices()==0);
  std::printf("Real WAV rapid-hit simulation: hits=%u peak=%u clips=%u\n",hits,peak,clips);
  // Voice stealing never prevents a new hit.
  Mixer<2> two; two.begin(44100,true,false,8192);
  CHECK(two.trigger(steady,127,Group::None)==TriggerResult::Started); two.render(out,2);
  CHECK(two.trigger(steady,127,Group::None)==TriggerResult::Started);
  CHECK(two.trigger(steady,127,Group::None)==TriggerResult::Stolen); CHECK(two.activeVoices()==2);
  // Stop/restart mode cuts only the same Sample object, not another instrument.
  m.begin(44100,false,false,8192); m.trigger(steady,127,Group::None); m.trigger(s,127,Group::None);
  m.trigger(steady,127,Group::None); CHECK(m.activeVoices()==2);
  // Choke one cymbal, preserve another even when they share MIDI note 53.
  m.begin(44100,true,false,8192); m.trigger(steady,127,Group::Crash); m.trigger(steady,127,Group::Ride);
  m.choke(Group::Crash,220); m.render(out,110); m.choke(Group::Crash,220); m.render(out,110);
  CHECK(m.activeVoices()==1); m.choke(Group::Ride,0); CHECK(m.activeVoices()==0);
  // Open HH choke leaves kick alone.
  m.trigger(steady,127,Group::HihatOpen); m.trigger(steady,127,Group::None);
  m.choke(Group::HihatOpen,220); m.render(out,220); CHECK(m.activeVoices()==1);
  // Master ramp changes an ALREADY PLAYING sample and arrives at true zero.
  m.begin(44100,true,false,8192); m.trigger(steady,127,Group::None); m.render(out,1);
  const int16_t before=out[0]; m.setMasterGain(0,220); m.render(out,220);
  CHECK(out[0]<before && out[438]==0 && m.currentMasterGain()==0 && m.activeVoices()==1);
  m.render(out,64); for(int i=0;i<128;++i) CHECK(out[i]==0);
  m.setMasterGain(8192,220); m.render(out,100); const auto midway=m.currentMasterGain();
  m.setMasterGain(8192,220); m.render(out,120);
  CHECK(midway>0 && midway<8192 && m.currentMasterGain()==8192);
  // Audio velocity switch operates independently of the MIDI path.
  m.begin(44100,true,true,8192); m.trigger(steady,32,Group::None); m.render(out,1); const auto soft=out[0];
  m.stop(); m.trigger(steady,127,Group::None); m.render(out,1); CHECK(out[0]>soft*3);
  Sample bad=steady; bad.rate=48000; CHECK(m.trigger(bad,127,Group::None)==TriggerResult::Ignored);
  CHECK(m.trigger(steady,0,Group::None)==TriggerResult::Ignored);
  // Saturation is explicit, never integer wrap.
  auto loudBytes=wav(std::vector<int16_t>(10,32767)); auto loud=parse(loudBytes);
  m.begin(44100,true,false,32767); for(int i=0;i<16;++i)m.trigger(loud,127,Group::None);
  CHECK(m.render(out,1)==1 && out[0]==32767);
  // Volume control endpoints, monotonic curve, reverse, and smoothing.
  MasterVolumeFilter pot; uint16_t last=0;
  for(unsigned raw=0;raw<=4095;++raw) { pot.begin(raw); CHECK(pot.levelQ15()>=last); last=pot.levelQ15(); }
  pot.begin(0); CHECK(pot.levelQ15()==0 && pot.gainQ15(8192)==0);
  for(int i=0;i<200;++i)pot.update(4095);
  CHECK(pot.levelQ15()==32767 && pot.gainQ15(8192)==8192);
  for(int i=0;i<200;++i) { pot.update(0); }
  CHECK(pot.levelQ15()==0);
  pot.begin(0,32,4063,true); CHECK(pot.levelQ15()==32767);
  pot.begin(4095,32,4063,true); CHECK(pot.levelQ15()==0);
  pot.begin(2048); CHECK(pot.levelQ15()>8000 && pot.levelQ15()<8400);
  // Every truncation of a valid file is rejected with an empty output Sample.
  for(size_t n=0;n<b.size();++n) { Sample invalid=steady; CHECK(!parseWav(b.data(),n,invalid,error)); CHECK(!invalid.pcm); }
  for(auto offset: {20u,22u,32u,34u}) {
    auto broken=b; p16(broken,offset,0); Sample invalid; CHECK(!parseWav(broken.data(),broken.size(),invalid,error));
  }
  auto extra=b; extra.insert(extra.begin()+36,{'J','U','N','K',1,0,0,0,123,0}); p32(extra,4,extra.size()-8);
  CHECK(parseWav(extra.data(),extra.size(),s,error));
  auto missing=b; p32(missing,40,0xffffffffu); CHECK(!parseWav(missing.data(),missing.size(),s,error));
  // Fuzz mutations under ASan/UBSan: parser must never read outside supplied bytes.
  std::mt19937 rng(42);
  for(int i=0;i<10000;++i) { auto fuzz=b; for(int j=0;j<4;++j)fuzz[rng()%fuzz.size()]=rng()&255; parseWav(fuzz.data(),fuzz.size(),s,error); }
  std::printf("Engine/volume: %u assertions PASS; 10000 parser mutations completed\n",tests);
}
