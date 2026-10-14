#pragma once
#include <stdint.h>

// Pure control math, no ADC access here. Only loop() samples the potentiometer.
class MasterVolumeFilter {
 public:
  void begin(uint16_t raw, uint16_t low = 32, uint16_t high = 4063,
             bool reverse = false, bool squareCurve = true) {
    low_ = low;
    high_ = high > low ? high : static_cast<uint16_t>(low + 1);
    reverse_ = reverse;
    square_ = squareCurve;
    raw_ = raw > 4095u ? 4095 : raw;
    filteredQ8_ = static_cast<int32_t>(raw_) * 256;
  }
  void update(uint16_t raw) {
    raw_ = raw > 4095u ? 4095 : raw;
    // Exponential smoothing, alpha = 1/8, about 37 ms time constant at 5 ms.
    filteredQ8_ += (static_cast<int32_t>(raw_) * 256 - filteredQ8_) / 8;
  }
  uint16_t raw() const { return raw_; }
  uint16_t filtered() const {
    return static_cast<uint16_t>((filteredQ8_ + 128) / 256);
  }
  uint16_t levelQ15() const {
    const int32_t a = filtered();
    uint32_t q = a <= low_ ? 0u : (a >= high_ ? 32767u
        : static_cast<uint32_t>(a-low_) * 32767u / (high_-low_));
    if (reverse_) q = 32767u - q;
    if (square_) q = q * q / 32767u; // More control in the quiet range; not dB.
    return static_cast<uint16_t>(q);
  }
  uint16_t gainQ15(uint16_t ceiling) const {
    if (ceiling > 32767u) ceiling = 32767;
    return static_cast<uint16_t>(static_cast<uint32_t>(levelQ15()) * ceiling / 32767u);
  }
 private:
  uint16_t raw_ = 0, low_ = 32, high_ = 4063;
  int32_t filteredQ8_ = 0;
  bool reverse_ = false, square_ = true;
};
