#pragma once
#include "DrumEngine.h"
#include "Kick.h"
#include "HighTom.h"
#include "HighMidTom.h"
#include "FloorTom.h"
#include "SnareHead.h"
#include "SnareRim.h"
#include "SnareCross.h"
#include "HihatOpen.h"
#include "HihatClosed.h"
#include "HihatPedal.h"
#include "CrashBow.h"
#include "CrashEdge.h"
#include "CrashCup.h"
#include "RideBow.h"
#include "RideEdge.h"
#include "RideCup.h"

// Include additional converted WAV headers here, e.g.:
// #include "SnareHeadWav.h"

struct WavSlot {
  const uint8_t *bytes;
  size_t size;
};
// EXACT enum order from SampleId. nullptr means MIDI ONLY, not a fake sample.
// Replace a row with {SnareHeadWav, sizeof(SnareHeadWav)} to enable that sound.
static const WavSlot WAV_SLOTS[] = {
  {Kick, sizeof(Kick)}, // Kick
  {HighTom, sizeof(HighTom)}, // HighTom
  {HighMidTom, sizeof(HighMidTom)}, // HighMidTom
  {FloorTom, sizeof(FloorTom)}, // FloorTom
  {SnareHead, sizeof(SnareHead)}, // SnareHead
  {SnareRim, sizeof(SnareRim)}, // SnareRim
  {SnareCross, sizeof(SnareCross)}, // SnareCross
  {HihatOpen, sizeof(HihatOpen)}, // HihatOpen
  {HihatClosed, sizeof(HihatClosed)}, // HihatClosed
  {HihatPedal, sizeof(HihatPedal)}, // HihatPedal
  {CrashBow, sizeof(CrashBow)}, // CrashBow
  {CrashEdge, sizeof(CrashEdge)}, // CrashEdge
  {CrashCup, sizeof(CrashCup)}, // CrashCup
  {RideBow, sizeof(RideBow)}, // RideBow
  {RideEdge, sizeof(RideEdge)}, // RideEdge
  {RideCup, sizeof(RideCup)},  // RideCup
};
static const char * const SAMPLE_NAMES[] = {
  "Kick", "HighTom", "HighMidTom", "FloorTom", "SnareHead", "SnareRim",
  "SnareCross", "HihatOpen", "HihatClosed", "HihatPedal", "CrashBow",
  "CrashEdge", "CrashCup", "RideBow", "RideEdge", "RideCup"
};
static_assert(sizeof(WAV_SLOTS)/sizeof(WAV_SLOTS[0]) == drum_engine::SAMPLE_COUNT,
              "There must be one WAV row per SampleId");
static_assert(sizeof(SAMPLE_NAMES)/sizeof(SAMPLE_NAMES[0]) == drum_engine::SAMPLE_COUNT,
              "There must be one name per SampleId");
