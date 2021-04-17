#ifndef GAME_CONTROL_H_FILE
#define GAME_CONTROL_H_FILE

/**
 * The GameControl class handles the game logic processing,
 * like input, movement and collision.
 */

#include "game_data.h"
#include "game_joy.h"
#include "game_character.h"

class GameControl {
protected:
  int last_step_millis = 0;
  GameCharacter player;

  void screenFollowCharacter(GameCharacter &c);
  void moveShots();
  
public:
  void init() {
    const MAP_SPAWN_POINT *spawn = &game_map.spawn_points[0];
    player.init(&char_def, &game_sprites[0], spawn->pos.x>>16, spawn->pos.y>>16, spawn->dir);
  }
  void step(int cur_millis, GameJoy &joy);
  
};

#endif /* GAME_CONTROL_H_FILE */
