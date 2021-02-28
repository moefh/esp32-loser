
#include "game_control.h"
#include "game_joy.h"

#include "collision.h"

#define FRAME_DELAY 1

#define MAX_JUMP_SPEED   0x000e0000
#define START_JUMP_SPEED 0x000e0000
#define FALL_SPEED       0x00010000
#define INC_JUMP_SPEED   0x00007000

#define MAX_WALK_SPEED   0x00070000
#define DEC_WALK_SPEED   0x00008000

void GameControl::decreaseCharHorizontalSpeed(int amount)
{
  int sign;

  if (char_dx >= 0) {
    sign = 1;
  } else {
    char_dx = -char_dx;
    sign = -1;
  }
  char_dx -= amount;
  if (char_dx <= 0) {
    char_dx = 0;
    if (char_state == STATE_WALK) {
      char_state = STATE_STAND;
      char_frame = 0;
    }
  } else if (sign < 0) {
    char_dx = -char_dx;
  }
}

void GameControl::controlCharStand(SPRITE *spr, int dx, int jump, int last_jump) {
  if (jump && ! last_jump) {
    char_state = STATE_JUMP_START;
    char_dy = -START_JUMP_SPEED;
    char_frame = 0;
  } else if (dx != 0) {
    char_state = STATE_WALK;
    char_frame = 0;
  }
}

void GameControl::controlCharWalk(SPRITE *spr, int dx, int jump, int last_jump) {
  if (jump && ! last_jump) {
    char_state = STATE_JUMP_START;
    char_dy = -START_JUMP_SPEED;
    char_frame = 0;
  } else if (dx == 0) {
    char_state = STATE_STAND;
    char_frame = 0;
  }
}

void GameControl::controlCharJumpStart(SPRITE *spr, int dx, int jump, int last_jump) {
  if (jump) {
    char_dy -= INC_JUMP_SPEED;
  } else {
    char_state = STATE_JUMP_END;
    char_frame = 0;
  }
}

void GameControl::controlCharJumpEnd(SPRITE *spr, int dx, int jump, int last_jump) {
}

void GameControl::controlCharacter(SPRITE *spr, GameJoy &joy) {
  int dx =  (joy.cur_x - 2048) / 200;
  if (dx < -2) dx += 2; else if (dx > 2) dx -= 2; else dx = 0;
  //int dy = -(joy.cur_y - 2048) / 200;
  //if (dy < -2) dy += 2; else if (dy > 2) dy -= 2; else dy = 0;
  int jump = !joy.cur_c;
  int last_jump = !joy.last_c;

  if (dx > 0) {
    char_dx += 2*DEC_WALK_SPEED;
    char_dir = DIR_RIGHT;
  } else if (dx < 0) {
    char_dx -= 2*DEC_WALK_SPEED;
    char_dir = DIR_LEFT;
  }
  if (char_dx < -MAX_WALK_SPEED) char_dx = -MAX_WALK_SPEED;
  if (char_dx >  MAX_WALK_SPEED) char_dx =  MAX_WALK_SPEED;

  switch (char_state) {
  case STATE_STAND:      controlCharStand    (spr, dx, jump, last_jump); break;
  case STATE_WALK:       controlCharWalk     (spr, dx, jump, last_jump); break;
  case STATE_JUMP_START: controlCharJumpStart(spr, dx, jump, last_jump); break;
  case STATE_JUMP_END:   controlCharJumpEnd  (spr, dx, jump, last_jump); break;
  }
}

void GameControl::moveCharacter(SPRITE *spr, GameJoy &joy) {
  int dx, dy;
  if (char_state == STATE_STAND || char_state == STATE_WALK) {
    // check floor under character
    calc_movement(spr->x, spr->y, spr->def->width, spr->def->height, 0, 1, &dx, &dy);
    if (dy > 0) {
      // start falling
      char_state = STATE_JUMP_END;
      char_frame = 0;
    }
  }

  if (char_state != STATE_STAND && char_state != STATE_WALK) {
    char_dy += FALL_SPEED;
  }

  int flags = calc_movement(spr->x, spr->y, spr->def->width, spr->def->height, char_dx/0x10000, char_dy/0x10000, &dx, &dy);
  if (flags & CM_Y_CLIPPED) {
    if (char_dy > 0) {      /* Hit the ground */
      char_state = (joy.cur_x < 1000 || joy.cur_x > 3000) ? STATE_WALK : STATE_STAND;
      char_dy = 0;
      char_frame = 0;
    } else {                /* Hit the ceiling */
      char_dy = -char_dy;
      char_state = STATE_JUMP_END;
    }
  }
  if (flags & CM_X_CLIPPED)
    decreaseCharHorizontalSpeed(DEC_WALK_SPEED / 2);

  spr->x += dx;
  spr->y += dy;
  if (spr->x < 0) spr->x = 0;
  if (spr->y < 0) spr->y = 0;

  switch (char_state) {
  case STATE_STAND:  /* ??? */
  case STATE_WALK:
    decreaseCharHorizontalSpeed(DEC_WALK_SPEED);
    break;

  case STATE_JUMP_START:
    if (char_dy < 0 && char_dy > -FALL_SPEED)
      char_state = STATE_JUMP_END;
    if (char_dy > MAX_JUMP_SPEED)
      char_dy = MAX_JUMP_SPEED;
    break;

  case STATE_JUMP_END:
    if (char_dy > MAX_JUMP_SPEED)
      char_dy = MAX_JUMP_SPEED;
    break;
  }

  if (++char_frame_delay >= FRAME_DELAY) {
    char_frame++;
    char_frame_delay = 0;
  }
  spr->frame = calcCharSprFrame(spr);
}

int GameControl::calcCharSprFrame(SPRITE *spr) {
  switch (char_state) {
  case STATE_STAND: return char_def.stand[char_frame%char_def.num_stand] + ((char_dir==DIR_LEFT)?char_def.mirror:0);
  case STATE_WALK: return char_def.walk[char_frame%char_def.num_walk] + ((char_dir==DIR_LEFT)?char_def.mirror:0);
  case STATE_JUMP_START:
  case STATE_JUMP_END:
    return char_def.jump[char_frame%char_def.num_jump] + ((char_dir==DIR_LEFT)?char_def.mirror:0);
  }
  return 0;
}

void GameControl::step(int cur_millis, GameJoy &joy) {
  if (cur_millis - last_step_millis < 16) {
    return;
  }
  last_step_millis = cur_millis;
  
  controlCharacter(&sprites[0], joy);
  moveCharacter(&sprites[0], joy);
  joy.ack();
}
