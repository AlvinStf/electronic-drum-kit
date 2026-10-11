#pragma once
#include "UiProtocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
// ONLY these copied messages cross cores. No display access to HelloDrum,
// live settings arrays, Preferences, MIDI, the ADC or mixer internals.
namespace drum_ui {
static QueueHandle_t commandQueue=nullptr, snapshotQueue=nullptr;
inline bool createBridge() {
  commandQueue=xQueueCreate(8,sizeof(Command));
  snapshotQueue=xQueueCreate(1,sizeof(Snapshot));
  if(commandQueue && snapshotQueue) return true;
  if(commandQueue) vQueueDelete(commandQueue);
  if(snapshotQueue) vQueueDelete(snapshotQueue);
  commandQueue=nullptr;snapshotQueue=nullptr;
  return false;
}
inline void destroyBridge() {
  if(commandQueue) vQueueDelete(commandQueue);
  if(snapshotQueue) vQueueDelete(snapshotQueue);
  commandQueue=nullptr;snapshotQueue=nullptr;
}
} // namespace drum_ui
