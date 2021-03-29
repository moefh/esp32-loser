
#include "game_character.h"
#include "collision.h"

#define FRAME_DELAY 1

#define MAX_JUMP_SPEED   0x000e0000
#define START_JUMP_SPEED 0x000e0000
#define FALL_SPEED       0x00010000
#define INC_JUMP_SPEED   0x0000a000

#define MAX_WALK_SPEED   0x00070000
#define DEC_WALK_SPEED   0x0000b000

void GameCharacter::calcSpriteState()
{
  switch (state) {
  case STATE_STAND:    spr->frame = def->stand[frame % def->num_stand] + ((dir==DIR_LEFT) ? def->mirror : 0); break;
  case STATE_WALK:     spr->frame = def->walk [frame % def->num_walk]  + ((dir==DIR_LEFT) ? def->mirror : 0); break;
  case STATE_JUMP_START:
  case STATE_JUMP_END: spr->frame = def->jump [frame % def->num_jump]  + ((dir==DIR_LEFT) ? def->mirror : 0); break;
  default:             spr->frame = 0; break;
  }

  spr->x = x + ((dir == DIR_RIGHT) ? -def->clip.x : def->clip.x + def->clip.width - spr->def->width - 1);
  spr->y = y - def->clip.y;
}

void GameCharacter::decreaseHorizontalSpeed(int amount)
{
  int sign;

  if (dx >= 0) {
    sign = 1;
  } else {
    dx = -dx;
    sign = -1;
  }
  dx -= amount;
  if (dx <= 0) {
    dx = 0;
    if (state == STATE_WALK) {
      state = STATE_STAND;
      frame = 0;
    }
  } else if (sign < 0) {
    dx = -dx;
  }
}

void GameCharacter::controlStand(int joy_dx, int jump, int last_jump)
{
  if (jump && ! last_jump) {
    state = STATE_JUMP_START;
    dy = -START_JUMP_SPEED;
    frame = 0;
  } else if (joy_dx != 0) {
    state = STATE_WALK;
    frame = 0;
  }
}

void GameCharacter::controlWalk(int joy_dx, int jump, int last_jump)
{
  if (jump && ! last_jump) {
    state = STATE_JUMP_START;
    dy = -START_JUMP_SPEED;
    frame = 0;
  } else if (joy_dx == 0) {
    state = STATE_STAND;
    frame = 0;
  }
}

void GameCharacter::controlJumpStart(int joy_dx, int jump, int last_jump) {
  if (jump) {
    dy -= INC_JUMP_SPEED;
  } else {
    state = STATE_JUMP_END;
    frame = 0;
  }
}

void GameCharacter::controlJumpEnd(int joy_dx, int jump, int last_jump) {
  // nothing to do
}

void GameCharacter::control(GameJoy &joy) {
  int joy_dx = (joy.cur & JOY_BTN_LEFT) ? -1 : (joy.cur & JOY_BTN_RIGHT) ? 1 : 0;
  int jump = joy.cur & JOY_BTN_C;
  int last_jump = joy.last & JOY_BTN_C;

  if (joy_dx > 0) {
    dx += 2*DEC_WALK_SPEED;
    dir = DIR_RIGHT;
  } else if (joy_dx < 0) {
    dx -= 2*DEC_WALK_SPEED;
    dir = DIR_LEFT;
  }
  if (dx < -MAX_WALK_SPEED) dx = -MAX_WALK_SPEED;
  if (dx >  MAX_WALK_SPEED) dx =  MAX_WALK_SPEED;

  switch (state) {
  case STATE_STAND:      controlStand    (joy_dx, jump, last_jump); break;
  case STATE_WALK:       controlWalk     (joy_dx, jump, last_jump); break;
  case STATE_JUMP_START: controlJumpStart(joy_dx, jump, last_jump); break;
  case STATE_JUMP_END:   controlJumpEnd  (joy_dx, jump, last_jump); break;
  }
}

void GameCharacter::move()
{
  int mdx, mdy;
  if (state == STATE_STAND || state == STATE_WALK) {
    // check floor under character
    calc_movement(x, y, def->clip.width, def->clip.height, 0, 1, &mdx, &mdy);
    if (mdy > 0) {
      // start falling
      state = STATE_JUMP_END;
      frame = 0;
    }
  }

  if (state != STATE_STAND && state != STATE_WALK) {
    dy += FALL_SPEED;
  }

  int flags = calc_movement(x, y, def->clip.width, def->clip.height, dx/0x10000, dy/0x10000, &mdx, &mdy);
  if (flags & CM_Y_CLIPPED) {
    if (dy > 0) {      /* Hit the ground */
      //state = (joy.cur & (JOY_BTN_LEFT|JOY_BTN_RIGHT)) ? STATE_WALK : STATE_STAND;
      state = STATE_WALK;
      dy = 0;
      frame = 0;
    } else {           /* Hit the ceiling */
      //dy = -dy;
      dy = 0;
      state = STATE_JUMP_END;
    }
  }
  if (flags & CM_X_CLIPPED) {
    decreaseHorizontalSpeed(DEC_WALK_SPEED / 2);
  }

  x += mdx;
  y += mdy;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  switch (state) {
  case STATE_STAND:  /* ??? */
  case STATE_WALK:
    decreaseHorizontalSpeed(DEC_WALK_SPEED);
    break;

  case STATE_JUMP_START:
    if (dy < 0 && dy > -FALL_SPEED)
      state = STATE_JUMP_END;
    if (dy > MAX_JUMP_SPEED)
      dy = MAX_JUMP_SPEED;
    break;

  case STATE_JUMP_END:
    if (dy > MAX_JUMP_SPEED)
      dy = MAX_JUMP_SPEED;
    break;
  }

  if (++frame_delay >= FRAME_DELAY) {
    frame++;
    frame_delay = 0;
  }
}
