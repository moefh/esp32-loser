
#ifndef GAME_ARDUINO_JOY_H_FILE
#define GAME_ARDUINO_JOY_H_FILE

#include "game_joy.h"
#include "Arduino.h"

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
    cur_a = digitalRead(pin_a);
    cur_b = digitalRead(pin_b);
    cur_c = digitalRead(pin_c);
    cur_d = digitalRead(pin_d);
    cur_e = digitalRead(pin_e);
    cur_f = digitalRead(pin_f);

    cur_x = analogRead(pin_x);
    cur_y = analogRead(pin_y);
  }

};

#endif /* GAME_ARDUINO_JOY_H_FILE */
