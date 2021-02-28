#ifndef GAME_CONTROL_H_FILE
#define GAME_CONTROL_H_FILE

#include "game_data.h"
#include "game_joy.h"

class GameControl {
protected:
  int last_step_millis = 0;
  GAME_DATA *game_data;
  int num_sprites;
  SPRITE *sprites;

  int char_dx;
  int char_dy;
  int char_dir;
  int char_frame;
  int char_frame_delay;

  enum {
    DIR_RIGHT = 1,
    DIR_LEFT = -1,
  };

  enum {
    STATE_STAND,        /* Standing */
    STATE_WALK,         /* Walking */
    STATE_JUMP_START,   /* Starting jump (going up) */
    STATE_JUMP_END      /* Ending jump (going down) */
  };
  int char_state;

  void controlCharStand(SPRITE *spr, int dx, int jump, int last_jump);
  void controlCharWalk(SPRITE *spr, int dx, int jump, int last_jump);
  void controlCharJumpStart(SPRITE *spr, int dx, int jump, int last_jump);
  void controlCharJumpEnd(SPRITE *spr, int dx, int jump, int last_jump);
  void controlCharacter(SPRITE *spr, GameJoy &joy);

  void moveCharacter(SPRITE *spr, GameJoy &joy);
  void decreaseCharHorizontalSpeed(int amount);

  int calcCharSprFrame(SPRITE *spr);

public:
  void init() { char_state = STATE_STAND; char_dir = DIR_RIGHT; char_frame_delay = char_frame = char_dx = char_dy = 0; }
  void setData(GAME_DATA *d) { game_data = d; }
  void setSprites(int n, SPRITE *s) { num_sprites = n; sprites = s; }
  void step(int cur_millis, GameJoy &joy);
  
};

#endif /* GAME_CONTROL_H_FILE */
