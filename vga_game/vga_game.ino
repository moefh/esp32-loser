#include "game_data.h"
#include "game_control.h"
#include "game_screen.h"
#include "game_joy.h"
#include "game_wiimote.h"
#include "game_arduino_joy.h"

#define USE_WIIMOTE  1  /* 1=use wiimote via bluetooth, 0=use arduino joystick shield */

// Joystick input pins (for Arduino joystick shield)
#define PIN_JOY_A      13
#define PIN_JOY_B      12
#define PIN_JOY_C      14
#define PIN_JOY_D      27
#define PIN_JOY_E      26
#define PIN_JOY_F      25
#define PIN_JOY_X      33
#define PIN_JOY_Y      32

// VGA output pins
#define PIN_RED_LOW    21
#define PIN_RED_HIGH   22
#define PIN_GREEN_LOW  18
#define PIN_GREEN_HIGH 19
#define PIN_BLUE_LOW   4
#define PIN_BLUE_HIGH  5
#define PIN_HSYNC      23
#define PIN_VSYNC      15

static const int pin_config[] = {
  PIN_RED_LOW,
  PIN_RED_HIGH,
  PIN_GREEN_LOW,
  PIN_GREEN_HIGH,
  PIN_BLUE_LOW,
  PIN_BLUE_HIGH,
  PIN_HSYNC,
  PIN_VSYNC
};

GameScreen screen;
GameControl control;

#if USE_WIIMOTE
GameWiimote joystick;
#else
GameArduinoJoy joystick(PIN_JOY_A, PIN_JOY_B, PIN_JOY_C, PIN_JOY_D, PIN_JOY_E, PIN_JOY_F, PIN_JOY_X, PIN_JOY_Y);
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting...");
  
  joystick.init();
  control.init();

  screen.init(pin_config);
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
