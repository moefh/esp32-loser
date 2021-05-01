
#ifndef GAME_WII_WIRED_H_FILE
#define GAME_WII_WIRED_H_FILE

#include "game_joy.h"
#include "wii_i2c.h"

class GameWiiWired : public GameJoy {
protected:
  enum Type {
    UNKNOWN,
    NUNCHUCK,
    CLASSIC,
  };
  
  int pin_sda;
  int pin_scl;
  Type type;
  bool init_ok;

  Type read_controller_type() {
    const unsigned char *ident = wii_i2c_read_ident();
    if (! ident) return Type::UNKNOWN;
    switch (wii_i2c_decode_ident(ident)) {
    case WII_I2C_IDENT_NUNCHUCK: return NUNCHUCK;
    case WII_I2C_IDENT_CLASSIC:  return CLASSIC;
    default: return UNKNOWN;
    }
  }
  
public:
  GameWiiWired(int pin_sda, int pin_scl) : pin_sda(pin_sda), pin_scl(pin_scl) {
  }
  
  virtual void init() {
    cur = 0;
    last = 0;
    if (wii_i2c_init(pin_sda, pin_scl) == 0) {
      type = read_controller_type();
      //Serial.printf("Controller type is %d\n", type);
      wii_i2c_request_state();
      init_ok = true;
    } else {
      //Serial.printf("ERROR initializing wii i2c\n");
      init_ok = false;
    }
  }

  virtual void update() {
    if (! init_ok) return;
    
    last = cur;
    const unsigned char *data = wii_i2c_read_state();
    wii_i2c_request_state();
    switch (type) {
    case NUNCHUCK:
      {
        wii_i2c_nunchuck_state state;
        wii_i2c_decode_nunchuck(data, &state);
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
