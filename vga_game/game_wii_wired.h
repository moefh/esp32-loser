
#ifndef GAME_WII_WIRED_H_FILE
#define GAME_WII_WIRED_H_FILE

#include "game_joy.h"
#include "wii_i2c.h"

class GameWiiWired : public GameJoy {
protected:
  enum Type {
    NONE,
    UNKNOWN,
    NUNCHUK,
    CLASSIC,
  };

  int i2c_port;
  int pin_sda;
  int pin_scl;
  Type type;
  bool init_ok;

  Type get_controller_type(const unsigned char *ident) {
    if (! ident) return Type::NONE;
    switch (wii_i2c_decode_ident(ident)) {
    case WII_I2C_IDENT_NUNCHUK: return NUNCHUK;
    case WII_I2C_IDENT_CLASSIC: return CLASSIC;
    default: return UNKNOWN;
    }
  }
  
public:
  GameWiiWired(int i2c_port, int pin_sda, int pin_scl) : i2c_port(i2c_port), pin_sda(pin_sda), pin_scl(pin_scl) {
  }
  
  virtual void init() {
    cur = 0;
    last = 0;
    type = NONE;
    init_ok = false;
    
    if (wii_i2c_init(i2c_port, pin_sda, pin_scl) != 0) {
      return;
    }
    const unsigned char *ident = wii_i2c_read_ident();
    Type controller_type = get_controller_type(ident);

    // start read task on CPU 0 with 15ms delay between reads
    if (wii_i2c_start_read_task(0, 15) != 0) {
      return;
    }

    type = controller_type;
    init_ok = true;
  }

  virtual int getType() {
    return CONTROLLER_WII_WIRED;
  }
  
  virtual const char *getName() {
    switch (type) {
    case NONE:    return "not found";
    case UNKNOWN: return "unknown";
    case NUNCHUK: return "Wii Nunchuk";
    case CLASSIC: return "Wii Classic";
    }
    return "?";
  }
  
  virtual void update() {
    if (! init_ok) return;
    
    const unsigned char *data = wii_i2c_read_data_from_task();
    if (! data) return;
    last = cur;
    switch (type) {
    case NUNCHUK:
      {
        wii_i2c_nunchuk_state state;
        wii_i2c_decode_nunchuk(data, &state);
        cur = ((state.c ? JOY_BTN_C : 0) |  // C
               (state.z ? JOY_BTN_D : 0) |  // Z
               ((state.y >  32) ? JOY_BTN_UP    : 0) |  // up
               ((state.y < -32) ? JOY_BTN_DOWN  : 0) |  // down
               ((state.x < -32) ? JOY_BTN_LEFT  : 0) |  // left
               ((state.x >  32) ? JOY_BTN_RIGHT : 0));  // right
      }
      break;

    case CLASSIC:
      {
        wii_i2c_classic_state state;
        wii_i2c_decode_classic(data, &state);
        cur = ((state.x     ? JOY_BTN_A : 0) |   // X
               (state.y     ? JOY_BTN_D : 0) |   // Y
               (state.a     ? JOY_BTN_B : 0) |   // A
               (state.b     ? JOY_BTN_C : 0) |   // B
               (state.plus  ? JOY_BTN_E : 0) |   // +
               (state.minus ? JOY_BTN_F : 0) |   // -
               (state.left  ? JOY_BTN_LEFT  : 0) |
               (state.right ? JOY_BTN_RIGHT : 0) |
               (state.down  ? JOY_BTN_DOWN  : 0) |
               (state.up    ? JOY_BTN_UP    : 0));
      }
      break;

    default:
      // unknown/missing controller :(
      break;
    }
  }
};

#endif /* GAME_WII_WIRED_H_FILE */
