
#ifndef GAME_ARDUINO_JOY_H_FILE
#define GAME_ARDUINO_JOY_H_FILE

#include "game_joy.h"
#include "Arduino.h"

#define ARDUINO_JOY_DEADZONE_X  2048
#define ARDUINO_JOY_DEADZONE_Y  2048

class GameArduinoJoy : public GameJoy {
protected:
  int pin_a;
  int pin_b;
  int pin_c;
  int pin_d;
  int pin_e;
  int pin_f;
  int pin_x;
  int pin_y;

public:
  GameArduinoJoy(int a, int b, int c, int d, int e, int f, int x, int y) {
    pin_a = a;
    pin_b = b;
    pin_c = c;
    pin_d = d;
    pin_e = e;
    pin_f = f;
    pin_x = x;
    pin_y = y;
  }

  virtual void init() {
    pinMode(pin_a, INPUT_PULLUP);
    pinMode(pin_b, INPUT_PULLUP);
    pinMode(pin_c, INPUT_PULLUP);
    pinMode(pin_d, INPUT_PULLUP);
    pinMode(pin_e, INPUT_PULLUP);
    pinMode(pin_f, INPUT_PULLUP);
  
    pinMode(pin_x, INPUT);
    pinMode(pin_y, INPUT);
  }

  virtual void update() {
    last = cur;
    int x = analogRead(pin_x);
    int y = analogRead(pin_y);
    cur = ((digitalRead(pin_a) ? 0 : JOY_BTN_A) |
           (digitalRead(pin_b) ? 0 : JOY_BTN_B) |
           (digitalRead(pin_c) ? 0 : JOY_BTN_C) |
           (digitalRead(pin_d) ? 0 : JOY_BTN_D) |
           (digitalRead(pin_e) ? 0 : JOY_BTN_E) |
           (digitalRead(pin_f) ? 0 : JOY_BTN_F) |
           ((x < 2048 - ARDUINO_JOY_DEADZONE_X/2) ? JOY_BTN_LEFT  : 0) |
           ((x > 2048 + ARDUINO_JOY_DEADZONE_X/2) ? JOY_BTN_RIGHT : 0) |
           ((y < 2048 - ARDUINO_JOY_DEADZONE_Y/2) ? JOY_BTN_DOWN  : 0) |
           ((y > 2048 + ARDUINO_JOY_DEADZONE_Y/2) ? JOY_BTN_UP    : 0));
  }

};

#endif /* GAME_ARDUINO_JOY_H_FILE */
