#pragma once
#include "Arduino.h"
struct HelloDrum {
  int pin1=0,pin2=-1,curve=0;
  bool hit=false,hitRim=false,hitCup=false,choke=false;
  bool openHH=false,closeHH=false,moving=false;
  int velocity=100,pedalCC=0;
  std::vector<std::vector<int>> scans;
  explicit HelloDrum(byte a):pin1(a){}
  HelloDrum(byte a,byte b):pin1(a),pin2(b){}
  void setCurve(byte x){curve=x;}
  void singlePiezo(byte a,byte b,byte c,byte d){scans.push_back({1,a,b,c,d});}
  void dualPiezo(byte a,byte b,byte c,byte d,byte e,byte f){scans.push_back({2,a,b,c,d,e,f});}
  void HH(byte a,byte b,byte c,byte d){scans.push_back({3,a,b,c,d});}
  void hihatControl(byte a,byte b,byte c,byte d,byte e){scans.push_back({4,a,b,c,d,e});}
  void cymbal3zone(byte a,byte b,byte c,byte d,byte e,byte f){scans.push_back({5,a,b,c,d,e,f});}
};
