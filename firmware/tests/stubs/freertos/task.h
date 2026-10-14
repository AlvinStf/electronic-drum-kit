#pragma once
#include "FreeRTOS.h"
inline int xPortGetCoreID(){return 1;}
inline int xTaskCreatePinnedToCore(void(*)(void*),const char*,unsigned,void*,unsigned,void*,int){return pdPASS;}
inline void vTaskDelete(void*){}

inline void vTaskDelay(unsigned n) {fakeNowMs+=n;}
