#pragma once
#include <Arduino.h>
#include <AudioTools.h>
#include <atomic>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "DrumConfig.h"
#include "DrumEngine.h"
#include "MasterVolume.h"
#include "SampleBank.h"

// Include in the main .ino only. No MIDI calls in this namespace: audio failure
// and a missing WAV must never disable or change the original USB MIDI path.
namespace drum_runtime {
using drum_engine::SampleId;
using drum_engine::Group;
using drum_engine::TriggerResult;
namespace cfg = drum_config;

enum class EventType : uint8_t { Play, Choke };
struct AudioEvent {
  EventType type;
  uint8_t id;
  uint8_t velocity;
  uint8_t group;
};
static audio_tools::I2SStream i2s;
static drum_engine::Sample samples[drum_engine::SAMPLE_COUNT];
static drum_engine::Mixer<cfg::MAX_VOICES> mixer;
static MasterVolumeFilter volume;
static QueueHandle_t eventQueue = nullptr;
// ready is setup/loop-owned; task fault and counters cross cores atomically.
static bool ready = false;
static std::atomic<bool> audioFault{false};
static std::atomic<uint32_t> masterTargetQ15{0};
static std::atomic<uint32_t> startedHits{0}, stolenVoices{0};
static std::atomic<uint32_t> clippedFrames{0}, writeErrors{0}, activeVoices{0};
static std::atomic<uint32_t> peakVoices{0};
static uint32_t padHits = 0, testHits = 0, queuedHits = 0, droppedEvents = 0;
static uint32_t unmappedHits = 0, unavailableHits = 0, invalidEvents = 0;
static uint32_t slotHits[drum_engine::SAMPLE_COUNT] = {};
static uint32_t testRemaining = 0, nextTestMs = 0, maxLoopGapUs = 0;
static uint32_t lastPotMs = 0;
// loop-owned telemetry; UI receives a copied snapshot, never reads these.
static uint32_t lastPadMs = 0;
static uint8_t lastHitSlot = 255, lastHitVelocity = 0;
static uint16_t unscaledMasterQ15 = 0;
static uint8_t outputTrimPercent = 100;
static bool localAudioEnabled = true;
static std::atomic<bool> audioVelocityEnabled{cfg::USE_AUDIO_VELOCITY};
inline void setGainTarget(uint16_t unscaled) {
  unscaledMasterQ15 = unscaled;
  masterTargetQ15.store(localAudioEnabled
      ? static_cast<uint32_t>(unscaled) * outputTrimPercent / 100u : 0u,
      std::memory_order_relaxed);
}
inline void configureOutput(bool enabled, bool velocity, uint8_t trim) {
  localAudioEnabled = enabled;
  outputTrimPercent = trim <= 100 ? trim : 100;
  audioVelocityEnabled.store(velocity, std::memory_order_relaxed);
  setGainTarget(unscaledMasterQ15);
}
static unsigned loadedSamples = 0;
static constexpr uint32_t MASTER_RAMP_FRAMES = cfg::SAMPLE_RATE * cfg::MASTER_RAMP_MS / 1000u;
static constexpr uint32_t CHOKE_RAMP_FRAMES = cfg::SAMPLE_RATE * cfg::CHOKE_RAMP_MS / 1000u;

inline void initializeVolume() {
  if (cfg::USE_VOLUME_POT) {
    pinMode(cfg::MASTER_VOLUME_PIN, INPUT);
    // Per-pin attenuation only: DO NOT change piezo or pedal attenuation.
    // Leave the existing 12-bit analogRead resolution used by custom HelloDrum.
    analogSetPinAttenuation(cfg::MASTER_VOLUME_PIN, ADC_11db);
    volume.begin(analogRead(cfg::MASTER_VOLUME_PIN), cfg::POT_RAW_MIN,
                 cfg::POT_RAW_MAX, cfg::POT_REVERSED, cfg::POT_SQUARE_CURVE);
    setGainTarget(volume.gainQ15(cfg::MASTER_CEILING_Q15));
  } else {
    setGainTarget(static_cast<uint32_t>(cfg::FIXED_VOLUME_Q15) *
                  cfg::MASTER_CEILING_Q15 / 32767u);
  }
  lastPotMs = millis();
}
inline void updateVolume() {
  const uint32_t now = millis();
  if (!cfg::USE_VOLUME_POT || static_cast<uint32_t>(now-lastPotMs) < cfg::POT_INTERVAL_MS)
    return;
  lastPotMs = now;
  volume.update(analogRead(cfg::MASTER_VOLUME_PIN));
  setGainTarget(volume.gainQ15(cfg::MASTER_CEILING_Q15));
}
inline void audioTask(void *) {
  int16_t out[cfg::BLOCK_FRAMES * 2];
  for (;;) {
    mixer.setUseVelocity(audioVelocityEnabled.load(std::memory_order_relaxed));
    AudioEvent event{};
    for (unsigned n = 0; n < cfg::EVENT_CAPACITY; ++n) {
      if (xQueueReceive(eventQueue, &event, 0) != pdTRUE) break;
      if (event.type == EventType::Choke) {
        mixer.choke(static_cast<Group>(event.group), CHOKE_RAMP_FRAMES);
      } else if (event.type == EventType::Play && event.id < drum_engine::SAMPLE_COUNT) {
        const auto id = static_cast<SampleId>(event.id);
        const auto result = mixer.trigger(samples[event.id], event.velocity, drum_engine::groupFor(id));
        if (result != TriggerResult::Ignored) startedHits.fetch_add(1, std::memory_order_relaxed);
        if (result == TriggerResult::Stolen) stolenVoices.fetch_add(1, std::memory_order_relaxed);
      }
    }
    const uint32_t before = static_cast<uint32_t>(mixer.activeVoices());
    if (before > peakVoices.load(std::memory_order_relaxed))
      peakVoices.store(before, std::memory_order_relaxed);
    mixer.setMasterGain(static_cast<uint16_t>(masterTargetQ15.load(std::memory_order_relaxed)),
                        MASTER_RAMP_FRAMES);
    const uint32_t clips = mixer.render(out, cfg::BLOCK_FRAMES);
    if (clips) clippedFrames.fetch_add(clips, std::memory_order_relaxed);
    activeVoices.store(static_cast<uint32_t>(mixer.activeVoices()), std::memory_order_relaxed);
    // This potentially blocking write is confined to the dedicated AUDIO task.
    // Keep sending zero frames at idle so I2S clocks do not stop between hits.
    size_t sent = 0;
    const auto *bytes = reinterpret_cast<const uint8_t *>(out);
    while (sent < sizeof(out)) {
      const size_t remaining = sizeof(out)-sent;
      const size_t n = i2s.write(bytes+sent, remaining);
      if (n == 0 || n > remaining || (n % (2 * sizeof(int16_t))) != 0) {
        writeErrors.fetch_add(1, std::memory_order_relaxed);
        audioFault.store(true);
        i2s.end();
        vTaskDelete(nullptr);
        return;
      }
      sent += n;
    }
  }
}
inline bool begin() {
  initializeVolume();
  audio_tools::AudioToolsLogger.begin(Serial, audio_tools::AudioToolsLogLevel::Error);
  for (size_t i = 0; i < drum_engine::SAMPLE_COUNT; ++i) {
    const auto &slot = WAV_SLOTS[i];
    if (!slot.bytes && !slot.size) continue; // Intentionally MIDI-only slot.
    const char *error = nullptr;
    if (!drum_engine::parseWav(slot.bytes, slot.size, samples[i], error)) {
      Serial.printf("WAV disabled: %s: %s\n", SAMPLE_NAMES[i], error);
      continue;
    }
    if (samples[i].rate != cfg::SAMPLE_RATE) {
      Serial.printf("WAV disabled: %s: rate=%lu, need %lu Hz\n", SAMPLE_NAMES[i],
                    static_cast<unsigned long>(samples[i].rate),
                    static_cast<unsigned long>(cfg::SAMPLE_RATE));
      samples[i] = drum_engine::Sample{};
      continue;
    }
    ++loadedSamples;
  }
  if (!loadedSamples) {
    Serial.println("No valid local WAV. USB MIDI remains active.");
    return false;
  }
  mixer.begin(cfg::SAMPLE_RATE, cfg::OVERLAP_SAME_SAMPLE, cfg::USE_AUDIO_VELOCITY, 0);
  eventQueue = xQueueCreate(cfg::EVENT_CAPACITY, sizeof(AudioEvent));
  if (!eventQueue) {
    Serial.println("Audio event queue allocation failed; USB MIDI remains active.");
    return false;
  }
  auto config = i2s.defaultConfig(audio_tools::TX_MODE);
  config.sample_rate = cfg::SAMPLE_RATE;
  config.channels = 2;
  config.bits_per_sample = 16;
  config.pin_bck = cfg::BCLK_PIN;
  config.pin_ws = cfg::LRCK_PIN;
  config.pin_data = cfg::DIN_PIN;
  config.pin_mck = -1;
  config.buffer_size = 64;
  config.buffer_count = 4;
  if (!i2s.begin(config)) {
    Serial.println("I2S failed; USB MIDI remains active.");
    vQueueDelete(eventQueue);
    eventQueue = nullptr;
    return false;
  }
  BaseType_t audioCore = 0;
#if !defined(CONFIG_FREERTOS_UNICORE) || !CONFIG_FREERTOS_UNICORE
  audioCore = (xPortGetCoreID() == 0) ? 1 : 0;
#endif
  if (xTaskCreatePinnedToCore(audioTask, "drumAudio", 6144, nullptr, 3,
                              nullptr, audioCore) != pdPASS) {
    Serial.println("Audio task failed; USB MIDI remains active.");
    i2s.end();
    vQueueDelete(eventQueue);
    eventQueue = nullptr;
    return false;
  }
  ready = true;
  Serial.printf("Audio: %u/%u WAV slots, %lu Hz, %u voices, core %d\n", loadedSamples,
                static_cast<unsigned>(drum_engine::SAMPLE_COUNT),
                static_cast<unsigned long>(cfg::SAMPLE_RATE),
                static_cast<unsigned>(cfg::MAX_VOICES), static_cast<int>(audioCore));
  return true;
}
inline bool sendEvent(const AudioEvent &event) {
  if (!ready || audioFault.load()) return false;
  // Never wait for queue space; MIDI/sensing must continue if audio overloads.
  if (xQueueSend(eventQueue, &event, 0) != pdTRUE) {
    ++droppedEvents;
    return false;
  }
  return true;
}
inline void choke(Group group) {
  const AudioEvent event{EventType::Choke, 0, 0, static_cast<uint8_t>(group)};
  sendEvent(event);
}
inline bool play(SampleId id, int velocity) {
  const size_t slot = static_cast<size_t>(id);
  if (slot >= drum_engine::SAMPLE_COUNT) { ++invalidEvents; return false; }
  // Always choke an open hihat when closed, even if the closed/pedal WAV has
  // not been supplied. This is separate from the user's MIDI noteOff policy.
  if (drum_engine::closesHihat(id)) choke(Group::HihatOpen);
  if (!samples[slot].pcm) { ++unmappedHits; return false; }
  if (!ready || audioFault.load()) { ++unavailableHits; return false; }
  if (velocity < 1) velocity = 1;
  if (velocity > 127) velocity = 127;
  const AudioEvent event{EventType::Play, static_cast<uint8_t>(id),
                        static_cast<uint8_t>(velocity), 0};
  if (!sendEvent(event)) return false;
  ++queuedHits;
  return true;
}
inline void padSound(SampleId id, int velocity) {
  ++padHits;
  lastPadMs = millis();
  lastHitSlot = static_cast<uint8_t>(id);
  lastHitVelocity = static_cast<uint8_t>(velocity < 1 ? 1 : (velocity > 127 ? 127 : velocity));
  const size_t slot = static_cast<size_t>(id);
  if (slot < drum_engine::SAMPLE_COUNT) ++slotHits[slot];
  play(id, velocity);
}
inline void noteLoopStart() {
  static bool first = true;
  static uint32_t previousUs = 0;
  const uint32_t now = micros();
  if (!first) {
    const uint32_t gap = now-previousUs;
    if (gap > maxLoopGapUs) maxLoopGapUs = gap;
  }
  first = false;
  previousUs = now;
}
inline void printStatus() {
  Serial.printf("pad=%lu test=%lu queued=%lu started=%lu unmapped=%lu unavailable=%lu "
                "dropped=%lu active=%lu peak=%lu stolen=%lu clips=%lu writeErr=%lu "
                "gapUs=%lu ready=%u fault=%u pot=%u filtered=%u gainQ15=%lu\n",
      static_cast<unsigned long>(padHits), static_cast<unsigned long>(testHits),
      static_cast<unsigned long>(queuedHits), static_cast<unsigned long>(startedHits.load()),
      static_cast<unsigned long>(unmappedHits), static_cast<unsigned long>(unavailableHits),
      static_cast<unsigned long>(droppedEvents), static_cast<unsigned long>(activeVoices.load()),
      static_cast<unsigned long>(peakVoices.load()), static_cast<unsigned long>(stolenVoices.load()),
      static_cast<unsigned long>(clippedFrames.load()), static_cast<unsigned long>(writeErrors.load()),
      static_cast<unsigned long>(maxLoopGapUs), ready ? 1u : 0u, audioFault.load() ? 1u : 0u,
      static_cast<unsigned>(volume.raw()), static_cast<unsigned>(volume.filtered()),
      static_cast<unsigned long>(masterTargetQ15.load()));
}
inline void service() {
  updateVolume();
  // No per-hit logging. Request status after playing, since printing can block.
  if (Serial.available() > 0) {
    const int command = Serial.read();
    if (command == 'k' || command == 'K') {
      ++testHits;
      play(SampleId::Kick, 110); // Local audio test only, not a USB MIDI note.
    } else if (command == 't' || command == 'T') {
      testRemaining = cfg::TEST_HITS;
      nextTestMs = millis();
    } else if (command == 'x' || command == 'X') {
      testRemaining = 0;
    } else if (command == 's' || command == 'S') {
      printStatus();
    }
  }
  const uint32_t now = millis();
  if (testRemaining && static_cast<int32_t>(now-nextTestMs) >= 0) {
    ++testHits;
    play(SampleId::Kick, 110);
    --testRemaining;
    nextTestMs = now + cfg::TEST_INTERVAL_MS;
  }
  // No additional vTaskDelay here. The supplied FSRSensing already has delay(1)
  // once per loop, which is intentionally preserved rather than doubled.
}
} // namespace drum_runtime
