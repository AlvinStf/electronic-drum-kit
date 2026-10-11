#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Pure settings/schema/serialization: no Arduino, display, or shared live data.
namespace drum_settings {
constexpr size_t PAD_COUNT = 9, MAX_PARAMS = 10;
enum class PadKind : uint8_t { Single, Dual, Hihat, Pedal, Cymbal };
struct Param { const char *label; uint8_t offset, low, high; bool isCurve; };
struct PadSpec { const char *name; PadKind kind; uint8_t size, curveOffset; };
// Same ordering and layout as the original nine byte arrays. Do NOT reorder.
static const PadSpec PAD_SPECS[PAD_COUNT] = {
  {"High Tom",PadKind::Single,6,5}, {"Hi-Mid Tom",PadKind::Single,6,5},
  {"Floor Tom",PadKind::Single,6,5}, {"Snare",PadKind::Dual,10,6},
  {"Hihat",PadKind::Hihat,7,4}, {"Hihat Pedal",PadKind::Pedal,7,5},
  {"Crash",PadKind::Cymbal,10,6}, {"Ride",PadKind::Cymbal,10,6},
  {"Kick",PadKind::Single,6,5}
};
static const Param SINGLE_PARAMS[] = {
  {"Sensitivity",0,1,100,false}, {"Threshold",1,1,100,false},
  {"Scan Time (ms)",2,1,100,false}, {"Mask Time (ms)",3,1,100,false},
  {"Curve",5,0,4,true}, {"MIDI Note",4,0,127,false}
};
static const Param DUAL_PARAMS[] = {
  {"Sensitivity",0,1,100,false}, {"Threshold",1,1,100,false},
  {"Scan Time (ms)",2,1,100,false}, {"Mask Time (ms)",3,1,100,false},
  {"Rim Sensitivity",4,1,100,false}, {"Rim Threshold",5,1,100,false},
  {"Curve",6,0,4,true}, {"Note Head",7,0,127,false},
  {"Note Rim",8,0,127,false}, {"Note Cross Stick",9,0,127,false}
};
static const Param HIHAT_PARAMS[] = {
  {"Sensitivity",0,1,100,false}, {"Threshold",1,1,100,false},
  {"Scan Time (ms)",2,1,100,false}, {"Mask Time (ms)",3,1,100,false},
  {"Curve",4,0,4,true}, {"Note Open",5,0,127,false},
  {"Note Closed",6,0,127,false}
};
static const Param PEDAL_PARAMS[] = {
  {"Sensitivity",0,1,100,false}, {"Threshold",1,1,100,false},
  {"Scan Start",2,1,100,false}, {"Scan End",3,1,100,false},
  {"Pedal Sensitivity",4,1,100,false}, {"Curve",5,0,4,true},
  {"Note Pedal",6,0,127,false}
};
static const Param CYMBAL_PARAMS[] = {
  {"Sensitivity",0,1,100,false}, {"Threshold",1,1,100,false},
  {"Scan Time (ms)",2,1,100,false}, {"Mask Time (ms)",3,1,100,false},
  {"Edge Threshold",4,1,100,false}, {"Cup Threshold",5,1,100,false},
  {"Curve",6,0,4,true}, {"Note Bow",7,0,127,false},
  {"Note Edge",8,0,127,false}, {"Note Cup",9,0,127,false}
};
inline const Param *params(size_t pad) {
  if (pad>=PAD_COUNT) return nullptr;
  switch (PAD_SPECS[pad].kind) {
    case PadKind::Dual: return DUAL_PARAMS;
    case PadKind::Hihat: return HIHAT_PARAMS;
    case PadKind::Pedal: return PEDAL_PARAMS;
    case PadKind::Cymbal: return CYMBAL_PARAMS;
    default: return SINGLE_PARAMS;
  }
}
inline const char *curveName(unsigned v) {
  static const char * const names[]={"Linear","Exp 1","Exp 2","Log 1","Log 2"};
  return v<5 ? names[v] : "?";
}
struct Config {
  uint8_t pads[PAD_COUNT][MAX_PARAMS];
  uint8_t audioVelocity;
  uint8_t masterTrim; // 0..100 percent of the EXISTING pot/ceiling gain
  uint8_t localAudio; // output mute only, never changes MIDI
};
static_assert(sizeof(Config)==93, "Storage format has changed: bump version");
inline bool equal(const Config &a,const Config &b) {return memcmp(&a,&b,sizeof(a))==0;}
inline bool validate(const Config &c,const char *&error) {
  for (size_t p=0;p<PAD_COUNT;++p) {
    const Param *s=params(p);
    for (size_t i=0;i<PAD_SPECS[p].size;++i) {
      const int v=c.pads[p][s[i].offset];
      if (v<s[i].low || v>s[i].high) {error="Value out of range";return false;}
    }
    for (size_t i=PAD_SPECS[p].size;i<MAX_PARAMS;++i)
      if (c.pads[p][i]!=0) {error="Invalid unused bytes";return false;}
    if (c.pads[p][0]<=c.pads[p][1]) {error="Sens must exceed threshold";return false;}
    if (PAD_SPECS[p].kind==PadKind::Pedal && c.pads[p][2]>=c.pads[p][3]) {
      error="Scan start must be < end";return false;
    }
    if (PAD_SPECS[p].kind==PadKind::Cymbal && c.pads[p][4]>=c.pads[p][5]) {
      error="Edge must be < cup";return false;
    }
  }
  if (c.audioVelocity>1 || c.localAudio>1 || c.masterTrim>100) {
    error="Invalid output settings";return false;
  }
  error=nullptr;return true;
}
inline bool setPad(Config &c,size_t pad,size_t menuParam,int value,const char *&error) {
  if (pad>=PAD_COUNT || menuParam>=PAD_SPECS[pad].size || value<0 || value>255) {
    error="Invalid parameter";return false;
  }
  Config trial=c;trial.pads[pad][params(pad)[menuParam].offset]=static_cast<uint8_t>(value);
  if (!validate(trial,error)) return false;
  c=trial;return true;
}
enum class OutputParam : uint8_t { LocalAudio, Velocity, Trim };
inline bool setOutput(Config &c,OutputParam param,int value,const char *&error) {
  if (value<0 || value>100) {error="Invalid output value";return false;}
  Config trial=c;
  switch (param) {
    case OutputParam::LocalAudio: trial.localAudio=static_cast<uint8_t>(value);break;
    case OutputParam::Velocity: trial.audioVelocity=static_cast<uint8_t>(value);break;
    case OutputParam::Trim: trial.masterTrim=static_cast<uint8_t>(value);break;
    default: error="Unknown output parameter";return false;
  }
  if (!validate(trial,error)) return false;
  c=trial;return true;
}
inline uint8_t outputValue(const Config &c,OutputParam param) {
  switch(param) {
    case OutputParam::LocalAudio: return c.localAudio;
    case OutputParam::Velocity: return c.audioVelocity;
    default: return c.masterTrim;
  }
}
constexpr size_t BLOB_SIZE=8+sizeof(Config)+4;
inline uint32_t crc32(const uint8_t *p,size_t n) {
  uint32_t c=0xffffffffu;
  for (size_t i=0;i<n;++i) {
    c^=p[i];
    for(unsigned j=0;j<8;++j) c=(c>>1)^((0u-(c&1u))&0xedb88320u);
  }
  return ~c;
}
inline bool encode(const Config &c,uint8_t *blob,size_t length) {
  const char *error=nullptr;
  if(!blob || length!=BLOB_SIZE || !validate(c,error)) return false;
  blob[0]='D';blob[1]='R';blob[2]='M';blob[3]='S';blob[4]=1;blob[5]=0;
  blob[6]=static_cast<uint8_t>(sizeof(Config));blob[7]=0;
  memcpy(blob+8,&c,sizeof(c));
  const uint32_t crc=crc32(blob,BLOB_SIZE-4);
  for(unsigned i=0;i<4;++i) blob[BLOB_SIZE-4+i]=static_cast<uint8_t>(crc>>(8*i));
  return true;
}
inline bool decode(const uint8_t *blob,size_t length,Config &out) {
  if(!blob || length!=BLOB_SIZE || memcmp(blob,"DRMS",4)!=0 || blob[4]!=1 ||
     blob[5]!=0 || blob[6]!=sizeof(Config) || blob[7]!=0) return false;
  uint32_t expected=0;
  for(unsigned i=0;i<4;++i) expected|=static_cast<uint32_t>(blob[BLOB_SIZE-4+i])<<(8*i);
  if(crc32(blob,BLOB_SIZE-4)!=expected) return false;
  Config trial{};memcpy(&trial,blob+8,sizeof(trial));const char *error=nullptr;
  if(!validate(trial,error)) return false;
  out=trial;return true; // Never partially apply damaged flash data.
}
} // namespace drum_settings
