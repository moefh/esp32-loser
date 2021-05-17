
#ifndef GAME_WIIMOTE_H_FILE
#define GAME_WIIMOTE_H_FILE

#include "game_joy.h"
#include "Wiimote.h"

#define WIIMOTE_ROTATE_DPAD  1   // 1=rotate D-pad (hold horizontally), 0=no rotation

class GameWiimote : public GameJoy {
protected:
  Wiimote wiimote;
  uint16_t wiimote_state;
  bool wiimote_updated;
  
  static void callback(uint8_t number, uint8_t *data, size_t len, void *callback_data) {
    GameWiimote *self = (GameWiimote *) callback_data;
    if (len >= 4 && data[0] == 0xA1 && data[1] == 0x30) {
      self->wiimote_state = (data[2] << 8) | data[3];
      self->wiimote_updated = true;
    }
  }
  
public:
  virtual void init() {
    cur = 0;
    last = 0;
    wiimote_state = 0;
    wiimote_updated = 0;
    Wiimote::init();
    Wiimote::register_callback(1, GameWiimote::callback, this);
  }

  virtual int getType() { return CONTROLLER_WIIMOTE; }
  virtual const char *getName() { return "Wiimote"; }

  virtual void update() {
    last = cur;
    Wiimote::handle();
    if (wiimote_updated) {
      cur = (((wiimote_state & 0x0008) ? JOY_BTN_A     : 0) |  // A
             ((wiimote_state & 0x0004) ? JOY_BTN_B     : 0) |  // B
             ((wiimote_state & 0x0002) ? JOY_BTN_C     : 0) |  // 1
             ((wiimote_state & 0x0001) ? JOY_BTN_D     : 0) |  // 2
             ((wiimote_state & 0x1000) ? JOY_BTN_E     : 0) |  // +
             ((wiimote_state & 0x0010) ? JOY_BTN_F     : 0) |  // -
           //((wiimote_state & 0x0080) ? JOY_BTN_HOME  : 0) |  // home
#if WIIMOTE_ROTATE_DPAD
             ((wiimote_state & 0x0800) ? JOY_BTN_LEFT  : 0) |  // up
             ((wiimote_state & 0x0400) ? JOY_BTN_RIGHT : 0) |  // down
             ((wiimote_state & 0x0100) ? JOY_BTN_DOWN  : 0) |  // left
             ((wiimote_state & 0x0200) ? JOY_BTN_UP    : 0));  // right
#else
             ((wiimote_state & 0x0800) ? JOY_BTN_UP    : 0) |  // up
             ((wiimote_state & 0x0400) ? JOY_BTN_DOWN  : 0) |  // down
             ((wiimote_state & 0x0100) ? JOY_BTN_LEFT  : 0) |  // left
             ((wiimote_state & 0x0200) ? JOY_BTN_RIGHT : 0));  // right
#endif
      wiimote_updated = false;
    }
  }
};

#endif /* GAME_WIIMOTE_H_FILE */
