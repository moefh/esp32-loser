#include "game_data.h"
#include "game_network.h"
#include "game_control.h"
#include "game_screen.h"
#include "game_joy.h"
#include "game_wiimote.h"
#include "game_arduino_joy.h"
#include "game_wii_wired.h"

// main configurations
#define CONTROLLER_TYPE  CONTROLLER_WII_WIRED  // one of CONTROLLER_xxx from game_joy.h
#define ENABLE_NETWORK   1                     // 1=enabled, 0=disabled
#define DEFAULT_NETWORK_STATE  0               // 1=enabled, 0=disabled

// Joystick input pins (for Arduino joystick shield)
#define PIN_JOY_A      13
#define PIN_JOY_B      12
#define PIN_JOY_C      14
#define PIN_JOY_D      27
#define PIN_JOY_E      26
#define PIN_JOY_F      25
#define PIN_JOY_X      33
#define PIN_JOY_Y      32

// Wii wired controller config (for Wii nunchuck or classic controller)
#define PIN_WII_SDA    32
#define PIN_WII_SCL    33
#define WII_I2C_PORT   0  // ESP32 I2C port (0 or 1)

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
GameNetwork network;

#if   CONTROLLER_TYPE == CONTROLLER_WIIMOTE
GameWiimote joystick;
#elif CONTROLLER_TYPE == CONTROLLER_ARDUINO_JOY
GameArduinoJoy joystick(PIN_JOY_A, PIN_JOY_B, PIN_JOY_C, PIN_JOY_D, PIN_JOY_E, PIN_JOY_F, PIN_JOY_X, PIN_JOY_Y);
#elif CONTROLLER_TYPE == CONTROLLER_WII_WIRED
GameWiiWired joystick(WII_I2C_PORT, PIN_WII_SDA, PIN_WII_SCL);
#else
#error Please define CONTROLLER_TYPE to one of the controller types from game_joy.h
#endif

static void setup_serial()
{
#if ARDUINO_ARCH_ESP32
  Serial.begin(115200);
#endif
}

void setup()
{
  setup_serial();
  printf("Starting...\n");

  joystick.init();
  control.init();

  delay(500);
    
#if ENABLE_NETWORK
  joystick.update();
  if (joystick.cur & JOY_BTN_C) {
    printf("Starting network\n");
    network.init();
  } else {
    printf("Network disabled\n");
  }
#endif

  screen.init(pin_config, &network, &joystick);
  screen.clear();
}

void loop()
{
  int cur_millis = millis();
  joystick.update();
  control.step(cur_millis, joystick);
  network.step();
  screen.show(cur_millis);
}
