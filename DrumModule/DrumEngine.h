#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Allocation-free PCM16 WAV parser and polyphonic drum mixer.
// No Arduino or AudioTools dependency: parser and mixer can be host-tested.
namespace drum_engine {

struct Sample {
  const uint8_t *pcm = nullptr;
  size_t frames = 0;
  uint32_t rate = 0;
  uint16_t channels = 0;
};

inline uint16_t u16le(const uint8_t *p) {
  return static_cast<uint16_t>(p[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8);
}

inline uint32_t u32le(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

// Parse chunks instead of assuming every WAV has a 44-byte header.
// Accept only RIFF/WAVE, uncompressed PCM, mono/stereo, signed 16-bit little endian.
inline bool parseWav(const uint8_t *wav, size_t bytes, Sample &out,
                     const char *&error) {
  out = Sample{};
  error = nullptr;
  if (!wav || bytes < 12 || memcmp(wav, "RIFF", 4) != 0 ||
      memcmp(wav + 8, "WAVE", 4) != 0) {
    error = "Not a RIFF/WAVE file";
    return false;
  }
  const size_t riffBytes = u32le(wav + 4);
  if (riffBytes < 4 || riffBytes > bytes - 8) {
    error = "Truncated or invalid RIFF size";
    return false;
  }
  const size_t end = riffBytes + 8;
  bool haveFmt = false;
  bool haveData = false;
  uint16_t format = 0, channels = 0, bits = 0, align = 0;
  uint32_t rate = 0, byteRate = 0;
  const uint8_t *data = nullptr;
  size_t dataBytes = 0;

  for (size_t pos = 12; pos < end;) {
    if (end - pos < 8) {
      error = "Incomplete WAV chunk header";
      return false;
    }
    const uint8_t *chunk = wav + pos;
    const size_t size = u32le(chunk + 4);
    const size_t payload = pos + 8;
    if (size > end - payload) {
      error = "WAV chunk extends past RIFF boundary";
      return false;
    }
    if (memcmp(chunk, "fmt ", 4) == 0) {
      if (haveFmt || size < 16) {
        error = "Duplicate or incomplete fmt chunk";
        return false;
      }
      const uint8_t *p = wav + payload;
      format = u16le(p);
      channels = u16le(p + 2);
      rate = u32le(p + 4);
      byteRate = u32le(p + 8);
      align = u16le(p + 12);
      bits = u16le(p + 14);
      haveFmt = true;
    } else if (memcmp(chunk, "data", 4) == 0) {
      if (haveData) {
        error = "Multiple data chunks are not supported";
        return false;
      }
      data = wav + payload;
      dataBytes = size;
      haveData = true;
    }
    pos = payload + size;
    if (size & 1u) {
      if (pos == end) {
        error = "Missing WAV chunk padding";
        return false;
      }
      ++pos;
    }
  }

  if (!haveFmt || !haveData || format != 1 || (channels != 1 && channels != 2) || bits != 16 ||
      align != channels * 2u || rate < 8000 || rate > 96000 ||
      byteRate != rate * align || dataBytes == 0 || (dataBytes % align) != 0) {
    error = "Need PCM mono/stereo 16-bit WAV, 8-96 kHz, valid fmt/data";
    return false;
  }
  out.pcm = data;
  out.frames = dataBytes / align;
  out.rate = rate;
  out.channels = channels;
  return true;
}


// Independent identities, NOT MIDI note numbers. Crash cup and ride cup both
// use MIDI note 53 in the user's sketch, but must remain separate audio slots.
enum class SampleId : uint8_t {
  Kick, HighTom, HighMidTom, FloorTom, SnareHead, SnareRim, SnareCross,
  HihatOpen, HihatClosed, HihatPedal, CrashBow, CrashEdge, CrashCup,
  RideBow, RideEdge, RideCup, Count
};
constexpr size_t SAMPLE_COUNT = static_cast<size_t>(SampleId::Count);
enum class Group : uint8_t { None, HihatOpen, Crash, Ride };
enum class TriggerResult : uint8_t { Ignored, Started, Stolen };

inline Group groupFor(SampleId id) {
  switch (id) {
    case SampleId::HihatOpen: return Group::HihatOpen;
    case SampleId::CrashBow: case SampleId::CrashEdge: case SampleId::CrashCup:
      return Group::Crash;
    case SampleId::RideBow: case SampleId::RideEdge: case SampleId::RideCup:
      return Group::Ride;
    default: return Group::None;
  }
}
inline bool closesHihat(SampleId id) {
  return id == SampleId::HihatClosed || id == SampleId::HihatPedal;
}
inline int32_t s16le(const uint8_t *p) {
  const uint16_t u = u16le(p);
  return u < 32768u ? static_cast<int32_t>(u) : static_cast<int32_t>(u) - 65536;
}

// Access from the audio task ONLY after setup. The control task never touches
// voice positions or mixer gain; it sends events and an atomic gain target.
template <size_t VoiceCount>
class Mixer {
  static_assert(VoiceCount > 0 && VoiceCount <= 32, "Use 1-32 voices");
  struct Voice {
    const Sample *sample = nullptr;
    size_t position = 0;
    uint16_t gainQ15 = 0;
    int32_t envelopeQ15 = 32767;
    uint32_t releaseLeft = 0;
    Group group = Group::None;
    bool active = false;
  };
 public:
  void begin(uint32_t rate, bool overlap, bool useVelocity,
             uint16_t gainQ15 = 0) {
    rate_ = rate;
    overlap_ = overlap;
    useVelocity_ = useVelocity;
    masterQ15_ = targetQ15_ = gainQ15 > 32767u ? 32767 : gainQ15;
    masterRampLeft_ = 0;
    stop();
  }
  // Audio-task only. Affects new hits, not the gain of existing voices.
  void setUseVelocity(bool enabled) { useVelocity_ = enabled; }
  void stop() {
    for (size_t i = 0; i < VoiceCount; ++i) voices_[i] = Voice{};
  }
  void setMasterGain(uint16_t gainQ15, uint32_t rampFrames) {
    const int32_t g = gainQ15 > 32767u ? 32767 : gainQ15;
    if (g == targetQ15_) return; // Do not restart a ramp every audio block.
    targetQ15_ = g;
    masterRampLeft_ = rampFrames;
    if (!rampFrames) masterQ15_ = targetQ15_;
  }
  uint16_t currentMasterGain() const { return static_cast<uint16_t>(masterQ15_); }
  TriggerResult trigger(const Sample &s, uint8_t velocity, Group group) {
    if (!s.pcm || !s.frames || !velocity || s.rate != rate_ ||
        (s.channels != 1 && s.channels != 2)) return TriggerResult::Ignored;
    if (velocity > 127) velocity = 127;
    if (!overlap_) {
      // Only cut previous instances of THIS slot, not another drum.
      for (size_t i = 0; i < VoiceCount; ++i)
        if (voices_[i].sample == &s) voices_[i].active = false;
    }
    size_t slot = VoiceCount;
    for (size_t i = 0; i < VoiceCount; ++i) {
      if (!voices_[i].active) { slot = i; break; }
    }
    const bool stolen = slot == VoiceCount;
    if (stolen) {
      slot = 0;
      // All slots run at the same rate without resampling. Greatest playback
      // position therefore belongs to an oldest active voice.
      for (size_t i = 1; i < VoiceCount; ++i)
        if (voices_[i].position > voices_[slot].position) slot = i;
    }
    Voice &v = voices_[slot];
    v = Voice{};
    v.sample = &s;
    v.group = group;
    v.gainQ15 = useVelocity_
        ? static_cast<uint16_t>(static_cast<uint32_t>(velocity) * 32767u / 127u)
        : 32767;
    v.active = true;
    return stolen ? TriggerResult::Stolen : TriggerResult::Started;
  }
  void choke(Group group, uint32_t rampFrames) {
    if (group == Group::None) return;
    for (size_t i = 0; i < VoiceCount; ++i) {
      Voice &v = voices_[i];
      if (!v.active || v.group != group) continue;
      if (!rampFrames) v.active = false;
      else if (!v.releaseLeft) v.releaseLeft = rampFrames;
      // Repeated choke messages must NOT prolong an existing release.
    }
  }
  size_t activeVoices() const {
    size_t n = 0;
    for (size_t i = 0; i < VoiceCount; ++i) n += voices_[i].active ? 1 : 0;
    return n;
  }
  // Counts frames clipped in either output channel. Clamp prevents overflow,
  // NOT audible distortion. Reduce gain if this counter grows while playing.
  uint32_t render(int16_t *stereo, size_t frames) {
    uint32_t clipped = 0;
    for (size_t f = 0; f < frames; ++f) {
      if (masterRampLeft_) {
        masterQ15_ += (targetQ15_ - masterQ15_) /
                      static_cast<int32_t>(masterRampLeft_);
        if (--masterRampLeft_ == 0) masterQ15_ = targetQ15_;
      }
      int32_t sumL = 0, sumR = 0;
      for (size_t i = 0; i < VoiceCount; ++i) {
        Voice &v = voices_[i];
        if (!v.active) continue;
        if (v.releaseLeft) {
          v.envelopeQ15 -= v.envelopeQ15 / static_cast<int32_t>(v.releaseLeft);
          if (--v.releaseLeft == 0) {
            v.active = false;
            continue;
          }
        }
        const Sample &s = *v.sample;
        const uint8_t *p = s.pcm + v.position * s.channels * 2u;
        const int32_t l = s16le(p);
        const int32_t r = s.channels == 2 ? s16le(p + 2) : l;
        // Keep the old kick gain when there is no choke envelope.
        const int32_t g = v.envelopeQ15 == 32767 ? v.gainQ15
            : static_cast<int32_t>(v.gainQ15) * v.envelopeQ15 / 32768;
        sumL += l * g / 32768;
        sumR += r * g / 32768;
        if (++v.position >= s.frames) v.active = false;
      }
      int32_t l = static_cast<int32_t>(static_cast<int64_t>(sumL) * masterQ15_ / 32768);
      int32_t r = static_cast<int32_t>(static_cast<int64_t>(sumR) * masterQ15_ / 32768);
      if (l > 32767 || l < -32768 || r > 32767 || r < -32768) ++clipped;
      if (l > 32767) l = 32767;
      if (l < -32768) l = -32768;
      if (r > 32767) r = 32767;
      if (r < -32768) r = -32768;
      stereo[2*f] = static_cast<int16_t>(l);
      stereo[2*f+1] = static_cast<int16_t>(r);
    }
    return clipped;
  }
 private:
  Voice voices_[VoiceCount];
  uint32_t rate_ = 44100;
  bool overlap_ = true, useVelocity_ = false;
  int32_t masterQ15_ = 0, targetQ15_ = 0;
  uint32_t masterRampLeft_ = 0;
};
} // namespace drum_engine
