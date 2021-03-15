
#ifndef GAME_JOY_H_FILE
#define GAME_JOY_H_FILE

#include "Arduino.h"

class GameJoy {
public:
  int cur_a, last_a;
  int cur_b, last_b;
  int cur_c, last_c;
  int cur_d, last_d;
  int cur_e, last_e;
  int cur_f, last_f;
  int cur_x, last_x;
  int cur_y, last_y;

  virtual void init() = 0;
  virtual void update() = 0;

  void ack() {
    last_a = cur_a;
    last_b = cur_b;
    last_c = cur_c;
    last_d = cur_d;
    last_e = cur_e;
    last_f = cur_f;
    last_x = cur_x;
    last_y = cur_y;
  }

};

#endif /* GAME_JOY_H_FILE */
