#pragma once
#include "FreeRTOS.h"
struct FakeQueue {unsigned capacity;size_t itemSize;std::deque<std::vector<uint8_t>> items;};
using QueueHandle_t=FakeQueue*;
inline QueueHandle_t xQueueCreate(unsigned n,size_t bytes){return new FakeQueue{n,bytes,{}};}
inline void vQueueDelete(QueueHandle_t q){delete q;}
inline int xQueueSend(QueueHandle_t q,const void *p,unsigned ticks){
  assert(ticks==0);assert(q);if(q->items.size()==q->capacity)return 0;
  const auto *b=static_cast<const uint8_t*>(p);q->items.emplace_back(b,b+q->itemSize);return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t q,void *p,unsigned ticks){
  assert(ticks==0);assert(q);if(q->items.empty())return 0;
  std::memcpy(p,q->items.front().data(),q->itemSize);q->items.pop_front();return pdTRUE;
}

inline int xQueueOverwrite(QueueHandle_t q,const void *p) {
  assert(q && q->capacity==1);q->items.clear();return xQueueSend(q,p,0);
}
inline unsigned uxQueueMessagesWaiting(QueueHandle_t q) {return q->items.size();}
