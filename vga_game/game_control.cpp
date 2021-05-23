
#include "game_control.h"
#include "collision.h"

#define CAMERA_TETHER_X  40
#define CAMERA_TETHER_Y  60

void GameControl::screenFollowCharacter(GameCharacter &c)
{
  int x = c.getCenterX();
  if (game_data.camera_x < x - CAMERA_TETHER_X) game_data.camera_x = x - CAMERA_TETHER_X;
  if (game_data.camera_x > x + CAMERA_TETHER_X) game_data.camera_x = x + CAMERA_TETHER_X;

  int y = c.getCenterY();
  if (game_data.camera_y < y - CAMERA_TETHER_Y) game_data.camera_y = y - CAMERA_TETHER_Y;
  if (game_data.camera_y > y + CAMERA_TETHER_Y) game_data.camera_y = y + CAMERA_TETHER_Y;
}

void GameControl::moveShots()
{
  for (int i = GAME_NUM_SPRITE_FIRST_LOCAL_SHOT; i < GAME_NUM_SPRITE_FIRST_REMOTE_SHOT; i++) {
    if (! game_sprites[i].def) continue;
    int dx = (game_sprites[i].frame == 0) ? 12 : -12;
    int dy = 0;
    int flags = calc_movement(game_sprites[i].x, game_sprites[i].y,
                              game_sprites[i].def->width, game_sprites[i].def->height,
                              dx, dy, &dx, &dy);
    if (flags != 0) {
      game_sprites[i].def = nullptr;
    } else {
      game_sprites[i].x += dx;
      game_sprites[i].y += dy;
    }
  }
}

void GameControl::step(int cur_millis, GameJoy &joy)
{
  if (cur_millis - last_step_millis < 16) {
    return;
  }
  last_step_millis = cur_millis;
  
  moveShots();
  
  player.control(joy);
  player.move();

  player.calcSpriteState();
  screenFollowCharacter(player);
}
