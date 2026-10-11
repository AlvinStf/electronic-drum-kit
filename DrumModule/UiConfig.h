#pragma once
#include "DrumConfig.h"
namespace ui_config {
constexpr bool ENABLE_DISPLAY = true;
// Actual CS/DC/RST and button defines from the user's WORKING example.
constexpr int TFT_CS = 39, TFT_DC = 40, TFT_RST = 41;
constexpr int BTN_UP = 48, BTN_SELECT = 45, BTN_DOWN = 0;
// Explicit hardware SPI. MISO is NOT used; GPIO13 remains a pad input.
constexpr int TFT_SCK = 12, TFT_MOSI = 11;
constexpr uint8_t ROTATION = 3; // 280x240 landscape, same rotation as example.
constexpr uint32_t SPI_HZ = 20000000;
constexpr uint32_t SNAPSHOT_MS = 100;
constexpr uint32_t SAVE_QUIET_MS = 750;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;
constexpr uint32_t BUTTON_LONG_MS = 800;
constexpr uint32_t BUTTON_REPEAT_MS = 120;
constexpr uint32_t BUTTON_REPEAT_START_MS = 450;
constexpr int PINS[] = {TFT_CS,TFT_DC,TFT_RST,TFT_SCK,TFT_MOSI,
                        BTN_UP,BTN_SELECT,BTN_DOWN};
constexpr bool validPins() {
  for (unsigned i=0; i<sizeof(PINS)/sizeof(PINS[0]); ++i) {
    const int p=PINS[i];
    if (p<0 || p>48 || (p>21 && p<26) || p==46 || (p>=26 && p<=37) ||
        p==19 || p==20 || drum_config::usedByPad(p) ||
        p==drum_config::MASTER_VOLUME_PIN || p==drum_config::BCLK_PIN ||
        p==drum_config::LRCK_PIN || p==drum_config::DIN_PIN) return false;
    for (unsigned j=0; j<i; ++j) if (p==PINS[j]) return false;
  }
  return true;
}
static_assert(validPins(), "Display/buttons overlap a pad, I2S, USB, pot, or memory pin");
static_assert(ROTATION<4, "Use rotation 0..3");
// GPIO0/45 are strapping pins, retained deliberately from the tested example.
// Do not hold DOWN at reset. No external pull-up on GPIO45; see README.
} // namespace ui_config
