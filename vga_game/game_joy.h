
#ifndef GAME_JOY_H_FILE
#define GAME_JOY_H_FILE

#include "Arduino.h"

class GameJoy {
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
  int cur_a, last_a;
  int cur_b, last_b;
  int cur_c, last_c;
  int cur_d, last_d;
  int cur_e, last_e;
  int cur_f, last_f;
  int cur_x, last_x;
  int cur_y, last_y;

  void init(int a, int b, int c, int d, int e, int f, int x, int y) {
    pin_a = a;
    pin_b = b;
    pin_c = c;
    pin_d = d;
    pin_e = e;
    pin_f = f;
    pin_x = x;
    pin_y = y;

    pinMode(pin_a, INPUT_PULLUP);
    pinMode(pin_b, INPUT_PULLUP);
    pinMode(pin_c, INPUT_PULLUP);
    pinMode(pin_d, INPUT_PULLUP);
    pinMode(pin_e, INPUT_PULLUP);
    pinMode(pin_f, INPUT_PULLUP);
  
    pinMode(pin_x, INPUT);
    pinMode(pin_y, INPUT);
  }

  void update() {
#define UPDATE_BTN(btn) cur_##btn = digitalRead(pin_##btn);
    UPDATE_BTN(a);
    UPDATE_BTN(b);
    UPDATE_BTN(c);
    UPDATE_BTN(d);
    UPDATE_BTN(e);
    UPDATE_BTN(f);
#undef UPDATE_BTN

#define UPDATE_AXIS(ax) last_##ax = cur_##ax; cur_##ax = analogRead(pin_##ax);
    UPDATE_AXIS(x);
    UPDATE_AXIS(y);
#undef UPDATE_AXIS
  }

  void ack() {
#define ACK_BTN(btn) last_##btn = cur_##btn;
    ACK_BTN(a);
    ACK_BTN(b);
    ACK_BTN(c);
    ACK_BTN(d);
    ACK_BTN(e);
    ACK_BTN(f);
    ACK_BTN(x);
    ACK_BTN(y);
#undef ACK_BTN
  }

};

#endif /* GAME_JOY_H_FILE */
