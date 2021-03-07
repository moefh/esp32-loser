#ifndef GAME_CHARACTER_H_FILE
#define GAME_CHARACTER_H_FILE

#include "game_data.h"
#include "game_joy.h"

class GameCharacter {
protected:
  const CHAR_DEF *def;
  SPRITE *spr;

  int x, y;          /* collision position */
  int dx, dy;        /* movement direction */
  int state;         /* STATE_xxx */
  int dir;           /* DIR_xxx */
  int frame;
  int frame_delay;

  void decreaseHorizontalSpeed(int amount);
  void controlStand(int mdx, int jump, int last_jump);
  void controlWalk(int mdx, int jump, int last_jump);
  void controlJumpStart(int mdx, int jump, int last_jump);
  void controlJumpEnd(int mdx, int jump, int last_jump);

public:
  enum {
    DIR_LEFT  = 0,
    DIR_RIGHT = 1,
  };

  enum {
    STATE_STAND,        /* Standing */
    STATE_WALK,         /* Walking */
    STATE_JUMP_START,   /* Starting jump (going up) */
    STATE_JUMP_END      /* Ending jump (going down) */
  };

  void init(const CHAR_DEF *init_def, SPRITE *init_spr, int init_x, int init_y, int init_dir) {
    def = init_def;
    spr = init_spr;
    x = init_x;
    y = init_y;
    dir = init_dir;
    state = STATE_STAND;
    dx = dy = 0;
    frame = frame_delay = 0;
  }

  int getCenterX() { return x + def->clip.width/2; }
  int getCenterY() { return y + def->clip.height/2; }

  void calcSpriteState();
  void control(GameJoy &joy);
  void move(GameJoy &joy);
};

#endif /* GAME_CHARACTER_H_FILE */
