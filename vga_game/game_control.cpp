
#include "game_control.h"
#include "game_joy.h"

#define CAMERA_TETHER_X  40
#define CAMERA_TETHER_Y  60

void GameControl::screenFollowCharacter(GameCharacter &c)
{
  int x = c.getCenterX();
  if (game_data->camera_x < x - CAMERA_TETHER_X) game_data->camera_x = x - CAMERA_TETHER_X;
  if (game_data->camera_x > x + CAMERA_TETHER_X) game_data->camera_x = x + CAMERA_TETHER_X;

  int y = c.getCenterY();
  if (game_data->camera_y < y - CAMERA_TETHER_Y) game_data->camera_y = y - CAMERA_TETHER_Y;
  if (game_data->camera_y > y + CAMERA_TETHER_Y) game_data->camera_y = y + CAMERA_TETHER_Y;
}

void GameControl::step(int cur_millis, GameJoy &joy)
{
  if (cur_millis - last_step_millis < 16) {
    return;
  }
  last_step_millis = cur_millis;
  
  player.control(joy);
  player.move();
  joy.ack();

  player.calcSpriteState();
  screenFollowCharacter(player);
}
