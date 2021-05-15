
#ifndef GAME_JOY_H_FILE
#define GAME_JOY_H_FILE

#include "util.h"

// controller types
#define CONTROLLER_WIIMOTE      1
#define CONTROLLER_ARDUINO_JOY  2
#define CONTROLLER_WII_WIRED    3

// joystick button bit flags
#define JOY_BTN_A     (1u<<0)
#define JOY_BTN_B     (1u<<1)
#define JOY_BTN_C     (1u<<2)
#define JOY_BTN_D     (1u<<3)
#define JOY_BTN_E     (1u<<4)
#define JOY_BTN_F     (1u<<5)
#define JOY_BTN_LEFT  (1u<<6)
#define JOY_BTN_RIGHT (1u<<7)
#define JOY_BTN_UP    (1u<<8)
#define JOY_BTN_DOWN  (1u<<9)

class GameJoy {
public:
  uint32_t cur;
  uint32_t last;

  virtual void init() = 0;
  virtual void update() = 0;
};

#endif /* GAME_JOY_H_FILE */
