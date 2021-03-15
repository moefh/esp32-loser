
#ifndef GAME_WIIMOTE_H_FILE
#define GAME_WIIMOTE_H_FILE

#include "game_joy.h"
#include "Wiimote.h"

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
    cur_a = last_a = 0;
    cur_b = last_b = 0;
    cur_c = last_c = 0;
    cur_d = last_d = 0;
    cur_e = last_e = 0;
    cur_f = last_f = 0;
    cur_x = last_x = 2048;
    cur_y = last_y = 2048;
    wiimote_state = 0;
    wiimote_updated = 0;
    Wiimote::init();
    Wiimote::register_callback(1, GameWiimote::callback, this);
  }

  virtual void update() {
    Wiimote::handle();
    if (wiimote_updated) {
      cur_a = (wiimote_state & 0x0008) ? 0 : 1;  // A
      cur_b = (wiimote_state & 0x0004) ? 0 : 1;  // B
      cur_c = (wiimote_state & 0x0002) ? 0 : 1;  // 1
      cur_d = (wiimote_state & 0x0001) ? 0 : 1;  // 2
      cur_e = (wiimote_state & 0x1000) ? 0 : 1;  // +
      cur_f = (wiimote_state & 0x0010) ? 0 : 1;  // -
      cur_x = (wiimote_state & 0x0800) ? 0 : (wiimote_state & 0x0400) ? 4095 : 2048; // up/down
      cur_y = (wiimote_state & 0x0100) ? 0 : (wiimote_state & 0x0200) ? 4095 : 2048; // right/left
      wiimote_updated = false;
    }
  }
};

#endif /* GAME_WIIMOTE_H_FILE */
