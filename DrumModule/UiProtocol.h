#pragma once
#include "SettingsModel.h"
namespace drum_ui {
enum class CommandType : uint8_t { SetPad, SetOutput, Save, Reload, Defaults, TestKick };
struct Command {
  CommandType type;
  uint8_t pad, parameter, value;
  uint32_t serial;
};
struct Snapshot {
  drum_settings::Config config{};
  bool dirty=false, audioReady=false, audioFault=false;
  uint8_t lastVelocity=0, volumePercent=0, loadedSamples=0;
  uint32_t commandSerial=0, padHits=0, activeVoices=0, dropped=0, clips=0;
  uint32_t writeErrors=0, maxLoopGapUs=0;
  char lastName[20]="-";
  char message[40]="Default settings";
};
enum class Key : uint8_t { None, Up, Down, Select, Back };
} // namespace drum_ui
