#pragma once
#include <stddef.h>
#include <stdint.h>

namespace drum_config {
constexpr int BCLK_PIN = 42; // 
constexpr int LRCK_PIN = 47; // 
constexpr int DIN_PIN = 21;  //
constexpr int MASTER_VOLUME_PIN = 15;
constexpr bool USE_VOLUME_POT = true;
constexpr bool POT_REVERSED = false;
constexpr bool POT_SQUARE_CURVE = true;
constexpr uint16_t POT_RAW_MIN = 32;
constexpr uint16_t POT_RAW_MAX = 4063;
constexpr uint32_t POT_INTERVAL_MS = 5;
// Full knob = old kick master gain (0.25). 100% is NOT unity digital gain.
constexpr uint16_t MASTER_CEILING_Q15 = 8192;
// Used only when USE_VOLUME_POT=false; 32767 = 100% of the above ceiling.
constexpr uint16_t FIXED_VOLUME_Q15 = 32767;
constexpr uint32_t MASTER_RAMP_MS = 5;
constexpr uint32_t CHOKE_RAMP_MS = 5;
constexpr uint32_t SAMPLE_RATE = 44100; // All enabled WAVs must match. No SRC.
constexpr size_t MAX_VOICES = 16;
constexpr bool OVERLAP_SAME_SAMPLE = true;
// Preserve the previous working fixed audio level. MIDI velocity is ALWAYS
// unchanged. Set true for velocity-dependent gain on local WAV playback too.
constexpr bool USE_AUDIO_VELOCITY = false;
constexpr size_t BLOCK_FRAMES = 64;
constexpr unsigned EVENT_CAPACITY = 64;
constexpr uint32_t TEST_HITS = 16;
constexpr uint32_t TEST_INTERVAL_MS = 100;

constexpr bool usedByPad(int pin) {
  return pin == 1 || pin == 2 || pin == 3 || pin == 4 || pin == 5 || pin == 6 ||
         pin == 7 || pin == 8 || pin == 9 || pin == 10 || pin == 13 || pin == 14;
}
constexpr bool reservedForThisTarget(int pin) {
  return pin == 19 || pin == 20 || (pin >= 26 && pin <= 37) ||
         pin == 0 || pin == 3 || pin == 45 || pin == 46;
}
static_assert(!usedByPad(BCLK_PIN) && !usedByPad(LRCK_PIN) && !usedByPad(DIN_PIN),
              "I2S cannot share any original pad input");
static_assert(BCLK_PIN != LRCK_PIN && BCLK_PIN != DIN_PIN && LRCK_PIN != DIN_PIN,
              "I2S pins must be distinct");
static_assert(!reservedForThisTarget(BCLK_PIN) && !reservedForThisTarget(LRCK_PIN) &&
              !reservedForThisTarget(DIN_PIN), "Avoid USB, memory and strapping pins");
static_assert(MASTER_VOLUME_PIN >= 1 && MASTER_VOLUME_PIN <= 18 &&
              !usedByPad(MASTER_VOLUME_PIN) && MASTER_VOLUME_PIN != BCLK_PIN &&
              MASTER_VOLUME_PIN != LRCK_PIN && MASTER_VOLUME_PIN != DIN_PIN,
              "Pot needs a free ADC pin, not a pad/I2S/USB pin");
static_assert(POT_RAW_MIN < POT_RAW_MAX && POT_RAW_MAX <= 4095,
              "Invalid potentiometer calibration");
static_assert(MASTER_CEILING_Q15 <= 32767 && FIXED_VOLUME_Q15 <= 32767,
              "Q15 gain must be 0..32767");
static_assert(SAMPLE_RATE >= 8000 && SAMPLE_RATE <= 96000, "Invalid sample rate");
} // namespace drum_config
