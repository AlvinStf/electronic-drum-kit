#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <hellodrum.h>
#include "AudioRuntime.h"
#include "UiConfig.h"
#include "UiBridge.h"

namespace drum_settings {
struct PadBinding { uint8_t *values; HelloDrum *pad; };
static PadBinding bindings[PAD_COUNT]{};
static Config current{}, original{}, savedBaseline{};
static uint32_t lastPublishMs=0, lastCommandSerial=0;
static bool forcePublish=true;
static char message[40]="Default settings";
constexpr const char *NVS_NAMESPACE="drum_st7789";
constexpr const char *NVS_KEY="settings_v1";
inline void status(const char *text) {
  snprintf(message,sizeof(message),"%s",text);forcePublish=true;
}
inline bool readSaved(Config &out) {
  Preferences p;
  if(!p.begin(NVS_NAMESPACE,true)) return false;
  uint8_t blob[BLOB_SIZE]{};
  const bool readOk=p.getBytesLength(NVS_KEY)==sizeof(blob) &&
                   p.getBytes(NVS_KEY,blob,sizeof(blob))==sizeof(blob);
  p.end();
  return readOk && decode(blob,sizeof(blob),out);
}
inline bool writeSaved(const Config &c) {
  uint8_t blob[BLOB_SIZE]{};
  if(!encode(c,blob,sizeof(blob))) return false;
  Preferences p;if(!p.begin(NVS_NAMESPACE,false)) return false;
  const bool ok=p.putBytes(NVS_KEY,blob,sizeof(blob))==sizeof(blob);
  p.end();
  if(!ok) return false;
  Config verify{};
  return readSaved(verify) && equal(c,verify);
}
inline void apply() {
  // Called on loop/setup task ONLY, between full scans, never by display task.
  for(size_t i=0;i<PAD_COUNT;++i) {
    memcpy(bindings[i].values,current.pads[i],PAD_SPECS[i].size);
    bindings[i].pad->setCurve(current.pads[i][PAD_SPECS[i].curveOffset]);
  }
  drum_runtime::configureOutput(current.localAudio!=0,current.audioVelocity!=0,current.masterTrim);
}
inline void begin(const PadBinding (&b)[PAD_COUNT]) {
  for(size_t i=0;i<PAD_COUNT;++i) {
    bindings[i]=b[i];
    memcpy(original.pads[i],b[i].values,PAD_SPECS[i].size);
  }
  original.audioVelocity=drum_config::USE_AUDIO_VELOCITY ? 1 : 0;
  original.masterTrim=100;original.localAudio=1;
  current=original;savedBaseline=original;
  Config loaded{};
  if(readSaved(loaded)) {current=loaded;savedBaseline=loaded;status("Saved settings loaded");}
  else status("Defaults: no valid saved settings");
  apply();
}
inline bool quietForStorage() {
  using namespace drum_runtime;
  const bool running=ready && !audioFault.load(std::memory_order_relaxed);
  return (!padHits || static_cast<uint32_t>(millis()-lastPadMs)>=ui_config::SAVE_QUIET_MS) &&
    testRemaining==0 && (!running || (activeVoices.load(std::memory_order_relaxed)==0 &&
    (!eventQueue || uxQueueMessagesWaiting(eventQueue)==0)));
}
inline void process(const drum_ui::Command &c) {
  using drum_ui::CommandType;
  lastCommandSerial=c.serial;
  const char *error=nullptr;
  switch(c.type) {
    case CommandType::SetPad:
      if(setPad(current,c.pad,c.parameter,c.value,error)) {apply();status("Applied in RAM; SAVE to keep");}
      else status(error);
      break;
    case CommandType::SetOutput:
      if(setOutput(current,static_cast<OutputParam>(c.parameter),c.value,error)) {apply();status("Applied in RAM; SAVE to keep");}
      else status(error);
      break;
    case CommandType::Save:
      // NVS flash writes can pause both cores. Never auto-save during playing.
      // User must stop hitting, let all tails finish, then explicitly retry.
      if(!quietForStorage()) {status("Busy: stop playing, then retry");break;}
      if(writeSaved(current)) {savedBaseline=current;status("Settings saved and verified");}
      else status("SAVE FAILED: settings still in RAM");
      break;
    case CommandType::Reload: {
      if(!quietForStorage()) {status("Busy: stop playing, then retry");break;}
      Config loaded{};
      if(readSaved(loaded)) {current=loaded;savedBaseline=loaded;apply();status("Saved settings reloaded");}
      else status("No valid saved settings; unchanged");
      break;
    }
    case CommandType::Defaults:
      current=original;apply();status("Defaults in RAM; SAVE to keep");break;
    case CommandType::TestKick:
      ++drum_runtime::testHits;
      drum_runtime::play(drum_engine::SampleId::Kick,110);
      status("Local kick test; no MIDI sent");break;
    default:status("Unknown menu command");break;
  }
}
inline void serviceBeforeScan() {
  if(!drum_ui::commandQueue) return;
  drum_ui::Command command{};
  // Bound the work per scan, no waiting for the UI or for a held button.
  if(xQueueReceive(drum_ui::commandQueue,&command,0)==pdTRUE) process(command);
}
inline drum_ui::Snapshot snapshot() {
  using namespace drum_runtime;
  drum_ui::Snapshot s{};
  s.config=current;s.dirty=!equal(current,savedBaseline);
  s.audioReady=ready;s.audioFault=audioFault.load(std::memory_order_relaxed);
  s.commandSerial=lastCommandSerial;s.padHits=padHits;
  s.activeVoices=activeVoices.load(std::memory_order_relaxed);
  s.dropped=droppedEvents;s.clips=clippedFrames.load(std::memory_order_relaxed);
  s.writeErrors=writeErrors.load(std::memory_order_relaxed);s.maxLoopGapUs=maxLoopGapUs;
  s.loadedSamples=static_cast<uint8_t>(loadedSamples);s.lastVelocity=lastHitVelocity;
  // Display effective output gain relative to the old 0.25 ceiling, not dB.
  s.volumePercent=drum_config::MASTER_CEILING_Q15 ? static_cast<uint8_t>(
      masterTargetQ15.load(std::memory_order_relaxed)*100u/drum_config::MASTER_CEILING_Q15) : 0;
  snprintf(s.lastName,sizeof(s.lastName),"%s",lastHitSlot<drum_engine::SAMPLE_COUNT ? SAMPLE_NAMES[lastHitSlot] : "-");
  snprintf(s.message,sizeof(s.message),"%s",message);
  return s;
}
inline void publish() {
  if(!drum_ui::snapshotQueue) return;
  const uint32_t now=millis();
  if(!forcePublish && static_cast<uint32_t>(now-lastPublishMs)<ui_config::SNAPSHOT_MS) return;
  const auto s=snapshot();
  xQueueOverwrite(drum_ui::snapshotQueue,&s); // Length-one mailbox; never wait.
  lastPublishMs=now;forcePublish=false;
}
} // namespace drum_settings
