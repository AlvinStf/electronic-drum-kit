#pragma once
#include "UiProtocol.h"
#include "UiConfig.h"

namespace drum_ui {
// All button timing uses wrap-safe subtraction; no delay/wait-for-release loop.
class DebouncedButton {
 public:
  explicit DebouncedButton(bool selectButton=false): select_(selectButton) {}
  void begin(bool down,uint32_t now) {
    raw_=stable_=down;changed_=pressed_=repeated_=now;
    // Ignore a button held at startup until it is released.
    armed_=!down;longSent_=false;
  }
  Key poll(bool down,uint32_t now,Key direction) {
    if(down!=raw_) {raw_=down;changed_=now;}
    if(raw_!=stable_ && static_cast<uint32_t>(now-changed_)>=ui_config::BUTTON_DEBOUNCE_MS) {
      stable_=raw_;
      if(stable_) {
        pressed_=repeated_=now;longSent_=false;
        if(armed_ && !select_) return direction;
      } else {
        const bool shortPress=armed_ && select_ && !longSent_;
        armed_=true;
        if(shortPress) return Key::Select;
      }
    }
    if(!stable_ || !armed_) return Key::None;
    if(select_) {
      if(!longSent_ && static_cast<uint32_t>(now-pressed_)>=ui_config::BUTTON_LONG_MS) {
        longSent_=true;return Key::Back;
      }
    } else if(static_cast<uint32_t>(now-pressed_)>=ui_config::BUTTON_REPEAT_START_MS &&
              static_cast<uint32_t>(now-repeated_)>=ui_config::BUTTON_REPEAT_MS) {
      repeated_=now;return direction;
    }
    return Key::None;
  }
 private:
  bool select_=false, raw_=false, stable_=false, armed_=true, longSent_=false;
  uint32_t changed_=0,pressed_=0,repeated_=0;
};

enum class Page : uint8_t { Play, Main, Pads, Pad, Output, Save, Reload, Defaults, About };
struct MenuLine { char label[28]{};char value[16]{};uint8_t style=0; };
struct MenuView {
  char title[32]{}, subtitle[44]{}, footer1[44]{}, footer2[44]{};
  MenuLine lines[6]{};
  uint8_t count=0;
};
class MenuController {
 public:
  Page page() const {return page_;}
  unsigned cursor() const {return cursor_;}
  unsigned selectedPad() const {return pad_;}
  bool editing() const {return editing_;}
  int draft() const {return draft_;}
  bool pending() const {return pending_;}
  void acknowledge(const Snapshot &s) {
    if(pending_ && s.commandSerial==serial_) pending_=false;
  }
  void queueFailed() {pending_=false;notice_="Menu busy; try again";}
  bool input(Key key,const Snapshot &s,Command &out) {
    acknowledge(s);
    if(key==Key::None || pending_) return false;
    notice_=nullptr;
    if(key==Key::Back) {
      if(editing_) {editing_=false;return false;}
      back();return false;
    }
    if(editing_) {
      if(key==Key::Up || key==Key::Down) {
        const int delta=key==Key::Up ? 1 : -1;
        const int high=page_==Page::Pad ? drum_settings::params(pad_)[cursor_].high : (cursor_==2 ? 100 : 1);
        const int low=page_==Page::Pad ? drum_settings::params(pad_)[cursor_].low : 0;
        draft_+=delta;if(draft_<low) draft_=low;if(draft_>high) draft_=high;
      } else if(key==Key::Select) {
        auto trial=s.config;const char *error=nullptr;
        const bool ok=page_==Page::Pad ? drum_settings::setPad(trial,pad_,cursor_,draft_,error) :
          drum_settings::setOutput(trial,static_cast<drum_settings::OutputParam>(cursor_),draft_,error);
        if(!ok) {notice_=error;return false;}
        out={page_==Page::Pad ? CommandType::SetPad : CommandType::SetOutput,
             static_cast<uint8_t>(pad_),static_cast<uint8_t>(cursor_),static_cast<uint8_t>(draft_),++serial_};
        editing_=false;pending_=true;return true;
      }
      return false;
    }
    if(page_==Page::Play) {if(key==Key::Select) go(Page::Main);return false;}
    if(key==Key::Up || key==Key::Down) {
      const unsigned n=itemCount();
      cursor_=key==Key::Up ? (cursor_+n-1)%n : (cursor_+1)%n;
      return false;
    }
    if(key!=Key::Select) return false;
    switch(page_) {
      case Page::Main:
        switch(cursor_) {
          case 0:go(Page::Pads);break;
          case 1:go(Page::Output);break;
          case 2:go(Page::Save);break;
          case 3:go(Page::Reload);break;
          case 4:go(Page::Defaults);break;
          case 5:go(Page::About);break;
          default:go(Page::Play);break;
        } break;
      case Page::Pads:
        if(cursor_==drum_settings::PAD_COUNT) back();
        else {pad_=PAD_ORDER[cursor_];go(Page::Pad);} break;
      case Page::Pad:
        if(cursor_==drum_settings::PAD_SPECS[pad_].size) back();
        else {draft_=s.config.pads[pad_][drum_settings::params(pad_)[cursor_].offset];editing_=true;}
        break;
      case Page::Output:
        if(cursor_<3) {draft_=drum_settings::outputValue(s.config,static_cast<drum_settings::OutputParam>(cursor_));editing_=true;}
        else if(cursor_==3) return command(CommandType::TestKick,out);
        else back();
        break;
      case Page::Save:case Page::Reload:case Page::Defaults:
        if(cursor_==0) {back();break;}
        {
          const auto t=page_==Page::Save ? CommandType::Save :
            (page_==Page::Reload ? CommandType::Reload : CommandType::Defaults);
          go(Page::Main);return command(t,out);
        }
      case Page::About:back();break;
      default:break;
    }
    return false;
  }
  MenuView view(const Snapshot &s,unsigned visibleRows=5) const {
    MenuView v{};if(visibleRows>6)visibleRows=6;
    if(visibleRows<1)visibleRows=1;
    const char *title="CY-PHYSIC E-DRUM";
    switch(page_) {
      case Page::Main:title="SETTINGS";break;
      case Page::Pads:title="SELECT PAD";break;
      case Page::Pad:title=drum_settings::PAD_SPECS[pad_].name;break;
      case Page::Output:title="OUTPUT";break;
      case Page::Save:title="SAVE TO FLASH?";break;
      case Page::Reload:title="RELOAD SAVED?";break;
      case Page::Defaults:title="RESTORE DEFAULTS?";break;
      case Page::About:title="ABOUT";break;
      default:break;
    }
    snprintf(v.title,sizeof(v.title),"%s%s",title,s.dirty ? " *" : "");
    if(page_==Page::Play) {
      snprintf(v.subtitle,sizeof(v.subtitle),"MIDI CH10 | WAV %u/16 | %s",s.loadedSamples,
          s.audioFault ? "FAULT" : (s.audioReady ? "READY" : "OFFLINE"));
      line(v.lines[0],"Last hit",s.lastName);
      number(v.lines[1],"Velocity",s.lastVelocity);
      char level[16];snprintf(level,sizeof(level),"%u%%",s.volumePercent);
      line(v.lines[2],"Master level",level);
      number(v.lines[3],"Active voices",s.activeVoices);
      char counters[16];snprintf(counters,sizeof(counters),"%lu/%lu",
          static_cast<unsigned long>(s.dropped%100000u),static_cast<unsigned long>(s.writeErrors%100000u));
      line(v.lines[4],"Drop / I2S error",counters);
      v.count=5;
    } else if(page_==Page::About) {
      snprintf(v.subtitle,sizeof(v.subtitle),"E-Drum v1");
      line(v.lines[0],"Board","S3 N16R8");line(v.lines[1],"Audio","44.1k / 16b");
      number(v.lines[2],"WAV slots ready",s.loadedSamples);
      number(v.lines[3],"Max loop gap us",s.maxLoopGapUs);
      line(v.lines[4],"< Back","");v.lines[4].style=1;v.count=5;
    } else {
      const unsigned count=itemCount();
      const unsigned first=(cursor_/visibleRows)*visibleRows;
      if(page_==Page::Save) snprintf(v.subtitle,sizeof(v.subtitle),"STOP PLAYING before saving");
      else if(page_==Page::Reload) snprintf(v.subtitle,sizeof(v.subtitle),"Discard unsaved changes?");
      else if(page_==Page::Defaults) snprintf(v.subtitle,sizeof(v.subtitle),"Original sketch values, RAM only");
      else snprintf(v.subtitle,sizeof(v.subtitle),"%s | %u/%u",editing_ ? "EDIT: SELECT to apply" : "UP / DOWN to navigate",cursor_+1,count);
      for(unsigned i=first;i<count && v.count<visibleRows;++i) {
        MenuLine &r=v.lines[v.count++];item(r,i,s);
        if(i==cursor_) r.style=editing_ ? 2 : 1;
      }
    }
    snprintf(v.footer1,sizeof(v.footer1),"%s",notice_ ? notice_ : (pending_ ? "Applying..." : s.message));
    snprintf(v.footer2,sizeof(v.footer2),"%s",page_==Page::Play ? "SELECT: menu | *: not saved" :
       (editing_ ? "UP/DN: value | hold SELECT: cancel" : "SELECT: enter | hold SELECT: back"));
    return v;
  }
 private:
  static constexpr uint8_t PAD_ORDER[9]={8,3,0,1,2,4,5,6,7};
  Page page_=Page::Play;unsigned cursor_=0,pad_=8;
  bool editing_=false,pending_=false;int draft_=0;uint32_t serial_=0;
  const char *notice_=nullptr;
  bool command(CommandType t,Command &out) {
    out={t,0,0,0,++serial_};pending_=true;return true;
  }
  void go(Page p) {page_=p;cursor_=0;editing_=false;notice_=nullptr;}
  void back() {
    if(page_==Page::Pad) go(Page::Pads);
    else if(page_==Page::Main || page_==Page::Play) go(Page::Play);
    else go(Page::Main);
  }
  unsigned itemCount() const {
    switch(page_) {
      case Page::Main:return 7;
      case Page::Pads:return drum_settings::PAD_COUNT+1;
      case Page::Pad:return drum_settings::PAD_SPECS[pad_].size+1;
      case Page::Output:return 5;
      case Page::Save:case Page::Reload:case Page::Defaults:return 2;
      default:return 1;
    }
  }
  static void line(MenuLine &r,const char *label,const char *value) {
    snprintf(r.label,sizeof(r.label),"%s",label);snprintf(r.value,sizeof(r.value),"%s",value);
  }
  static void number(MenuLine &r,const char *label,uint32_t value) {
    snprintf(r.label,sizeof(r.label),"%s",label);
    snprintf(r.value,sizeof(r.value),"%lu",static_cast<unsigned long>(value));
  }
  void item(MenuLine &r,unsigned i,const Snapshot &s) const {
    static const char * const mainItems[]={"Pad settings","Output","Save settings","Reload saved","Restore defaults","About","< Play"};
    if(page_==Page::Main) {line(r,mainItems[i],"");return;}
    if(page_==Page::Pads) {line(r,i<drum_settings::PAD_COUNT ? drum_settings::PAD_SPECS[PAD_ORDER[i]].name : "< Back","");return;}
    if(page_==Page::Pad) {
      if(i==drum_settings::PAD_SPECS[pad_].size) {line(r,"< Back","");return;}
      const auto &p=drum_settings::params(pad_)[i];
      const unsigned value=(editing_ && i==cursor_) ? draft_ : s.config.pads[pad_][p.offset];
      if(p.isCurve) line(r,p.label,drum_settings::curveName(value));else number(r,p.label,value);
      return;
    }
    if(page_==Page::Output) {
      static const char * const labels[]={"Local audio","Audio velocity","Master trim %","Sound Test","< Back"};
      if(i>=3) {line(r,labels[i],"");return;}
      const unsigned value=editing_ && i==cursor_ ? draft_ :
        drum_settings::outputValue(s.config,static_cast<drum_settings::OutputParam>(i));
      if(i<2) line(r,labels[i],value ? "On" : "Off");else number(r,labels[i],value);
      return;
    }
    line(r,i==0 ? "Cancel" : "Yes, continue","");
  }
};
// C++11/14 out-of-class definition for the constexpr array (one sketch TU).
constexpr uint8_t MenuController::PAD_ORDER[9];
} // namespace drum_ui
