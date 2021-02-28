#include "game_data.h"
#include "game_control.h"
#include "game_joy.h"

#include "game_screen_cpp.h"

#define JOY_A 13
#define JOY_B 12
#define JOY_C 14
#define JOY_D 27
#define JOY_E 26
#define JOY_F 25
#define JOY_X 33
#define JOY_Y 32

const PinConfig pinConfig(-1, -1, -1, 21, 22, -1, -1, -1, 18, 19, -1, -1, 4, 5, 23, 15, -1);

GameScreen screen;
GameControl control;
GameJoy joystick;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting...");
  
  joystick.init(JOY_A, JOY_B, JOY_C, JOY_D, JOY_E, JOY_F, JOY_X, JOY_Y);
  control.setData(&game_data);
  control.setSprites(game_num_sprites, game_sprites);
  control.init();

  delay(3000);
  screen.setFrameBufferCount(2);
  //vga.init(vga.MODE320x200, pinConfig);
  screen.init(screen.MODE320x240, pinConfig);
  screen.setFont(Font6x8);
  screen.setData(&game_data);
  screen.setSprites(game_num_sprites, game_sprites);
  screen.clear();
}

void loop()
{
  int cur_millis = millis();
  joystick.update();
  control.step(cur_millis, joystick);
  screen.show(cur_millis);
}
