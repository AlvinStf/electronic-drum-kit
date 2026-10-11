#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#ifndef DRUM_UI_HOST_TEST
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#endif
#include "freertos/task.h"
#include "UiConfig.h"
#include "UiBridge.h"
#include "MenuController.h"

namespace drum_ui {
// Only uiTask uses these peripherals. Do not use SPI elsewhere with this bus.
static SPIClass displaySPI(FSPI);
static Adafruit_ST7789 display(&displaySPI,ui_config::TFT_CS,ui_config::TFT_DC,ui_config::TFT_RST);
static constexpr uint16_t BG=ST77XX_BLACK, TEXT=ST77XX_WHITE, TITLE=0x001F;
static constexpr uint16_t ROW=0x2104, HIGHLIGHT=ST77XX_CYAN, EDIT=ST77XX_YELLOW;

inline void fitText(Adafruit_GFX &g,const char *src,int x,int baseline,int maxWidth) {
  char text[48];snprintf(text,sizeof(text),"%s",src);
  int16_t bx,by;uint16_t width,height;
  size_t n=strlen(text);
  // Clip long names without allowing wrapping or writing over value columns.
  for(;;) {
    g.getTextBounds(text,0,0,&bx,&by,&width,&height);
    if(static_cast<int>(width)<=maxWidth || n==0) break;
    text[--n]='\0';
  }
  g.setCursor(x-bx,baseline);g.print(text);
}
class Renderer {
 public:
  Renderer(Adafruit_ST7789 &d,GFXcanvas16 &stripe):d_(d),c_(stripe) {}
  void draw(const MenuView &v,unsigned visibleRows) {
    const int w=d_.width(),h=d_.height();
    if(!valid_ || strcmp(v.title,previous_.title)!=0) {
      c_.fillScreen(TITLE);c_.setFont(&FreeSansBold9pt7b);c_.setTextColor(TEXT);
      fitText(c_,v.title,8,23,w-16);blit(0,32);
    }
    if(!valid_ || strcmp(v.subtitle,previous_.subtitle)!=0) {
      c_.fillScreen(BG);c_.setFont(nullptr);c_.setTextColor(TEXT);
      fitText(c_,v.subtitle,7,4,w-14);blit(32,16);
    }
    for(unsigned i=0;i<visibleRows;++i) {
      if(valid_ && memcmp(&v.lines[i],&previous_.lines[i],sizeof(MenuLine))==0) continue;
      const auto &r=v.lines[i];
      const uint16_t bg=r.style==2 ? EDIT : (r.style==1 ? HIGHLIGHT : ROW);
      c_.fillScreen(BG);
      if(r.label[0]) {
        c_.fillRoundRect(4,1,w-8,26,4,bg);
        c_.setFont(&FreeSans9pt7b);c_.setTextColor(r.style ? ST77XX_BLACK : TEXT);
        int16_t bx,by;uint16_t vw,vh;
        c_.getTextBounds(r.value,0,0,&bx,&by,&vw,&vh);
        const int valueX=w-12-static_cast<int>(vw);
        fitText(c_,r.value,valueX,20,static_cast<int>(vw));
        fitText(c_,r.label,12,20,r.value[0] ? valueX-22 : w-24);
      }
      blit(48+static_cast<int>(i)*28,28);
    }
    if(!valid_ || strcmp(v.footer1,previous_.footer1)!=0 || strcmp(v.footer2,previous_.footer2)!=0) {
      c_.fillScreen(BG);c_.setFont(nullptr);c_.setTextColor(TEXT);
      fitText(c_,v.footer1,7,4,w-14);
      fitText(c_,v.footer2,7,19,w-14);blit(h-34,32);
    }
    previous_=v;valid_=true;
  }
 private:
  Adafruit_ST7789 &d_;GFXcanvas16 &c_;MenuView previous_{};bool valid_=false;
  void blit(int y,int height) {d_.drawRGBBitmap(0,y,c_.getBuffer(),d_.width(),height);}
};
inline void uiTask(void *) {
  // Explicit -1 MISO is important: no default SPI input attached to GPIO13.
  displaySPI.begin(ui_config::TFT_SCK,-1,ui_config::TFT_MOSI,ui_config::TFT_CS);
  display.init(240,280);
  display.setRotation(ui_config::ROTATION);
  display.setSPISpeed(ui_config::SPI_HZ);
  display.setTextWrap(false);display.fillScreen(BG);
  GFXcanvas16 stripe(display.width(),32);stripe.setTextWrap(false);
  if(!stripe.getBuffer()) {
    Serial.println("Display buffer allocation failed; MIDI/audio continue.");
    vTaskDelete(nullptr);return;
  }
  Renderer renderer(display,stripe);
  const unsigned visibleRows=static_cast<unsigned>((display.height()-48-42)/28);
  MenuController menu;Snapshot state{};
  DebouncedButton up(false),down(false),select(true);
  const uint32_t now=millis();
  up.begin(digitalRead(ui_config::BTN_UP)==LOW,now);
  down.begin(digitalRead(ui_config::BTN_DOWN)==LOW,now);
  select.begin(digitalRead(ui_config::BTN_SELECT)==LOW,now);
  for(;;) {
    if(xQueueReceive(snapshotQueue,&state,0)==pdTRUE) menu.acknowledge(state);
    const uint32_t t=millis();
    Key keys[]={up.poll(digitalRead(ui_config::BTN_UP)==LOW,t,Key::Up),
                down.poll(digitalRead(ui_config::BTN_DOWN)==LOW,t,Key::Down),
                select.poll(digitalRead(ui_config::BTN_SELECT)==LOW,t,Key::Select)};
    for(Key k:keys) {
      Command c{};
      if(menu.input(k,state,c) && xQueueSend(commandQueue,&c,0)!=pdTRUE) menu.queueFailed();
    }
    renderer.draw(menu.view(state,visibleRows),visibleRows);
    // This delay belongs to UI task only, NOT the pad-scanning task.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
inline bool begin(const Snapshot &initial) {
  if(!ui_config::ENABLE_DISPLAY) return false;
  if(!createBridge()) {Serial.println("UI queues failed; MIDI/audio continue.");return false;}
  pinMode(ui_config::BTN_UP,INPUT_PULLUP);pinMode(ui_config::BTN_DOWN,INPUT_PULLUP);
  pinMode(ui_config::BTN_SELECT,INPUT_PULLUP);
  xQueueOverwrite(snapshotQueue,&initial);
  BaseType_t uiCore=0;
#if !defined(CONFIG_FREERTOS_UNICORE) || !CONFIG_FREERTOS_UNICORE
  uiCore=(xPortGetCoreID()==0) ? 1 : 0;
#endif
  // Same other core as audio, but lower priority (UI=1, AUDIO=3).
  if(xTaskCreatePinnedToCore(uiTask,"drumUI",8192,nullptr,1,nullptr,uiCore)!=pdPASS) {
    destroyBridge();Serial.println("UI task failed; MIDI/audio continue.");return false;
  }
  return true;
}
} // namespace drum_ui
