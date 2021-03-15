
#include "game_character.h"
#include "collision.h"

#define FRAME_DELAY 1

#define MAX_JUMP_SPEED   0x000e0000
#define START_JUMP_SPEED 0x000e0000
#define FALL_SPEED       0x00010000
#define INC_JUMP_SPEED   0x0000a000

#define MAX_WALK_SPEED   0x00070000
#define DEC_WALK_SPEED   0x00008000

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

void GameCharacter::controlStand(int mdx, int jump, int last_jump)
{
  if (jump && ! last_jump) {
    state = STATE_JUMP_START;
    dy = -START_JUMP_SPEED;
    frame = 0;
  } else if (mdx != 0) {
    state = STATE_WALK;
    frame = 0;
  }
}

void GameCharacter::controlWalk(int mdx, int jump, int last_jump)
{
  if (jump && ! last_jump) {
    state = STATE_JUMP_START;
    dy = -START_JUMP_SPEED;
    frame = 0;
  } else if (mdx == 0) {
    state = STATE_STAND;
    frame = 0;
  }
}

void GameCharacter::controlJumpStart(int mdx, int jump, int last_jump) {
  if (jump) {
    dy -= INC_JUMP_SPEED;
  } else {
    state = STATE_JUMP_END;
    frame = 0;
  }
}

void GameCharacter::controlJumpEnd(int mdx, int jump, int last_jump) {
  // nothing to do
}

void GameCharacter::control(GameJoy &joy) {
  int mdx =  (joy.cur_x - 2048) / 200;
  if (mdx < -2) mdx += 2; else if (mdx > 2) mdx -= 2; else mdx = 0;
  int jump = ! joy.cur_c;
  int last_jump = ! joy.last_c;

  if (mdx > 0) {
    dx += 2*DEC_WALK_SPEED;
    dir = DIR_RIGHT;
  } else if (mdx < 0) {
    dx -= 2*DEC_WALK_SPEED;
    dir = DIR_LEFT;
  }
  if (dx < -MAX_WALK_SPEED) dx = -MAX_WALK_SPEED;
  if (dx >  MAX_WALK_SPEED) dx =  MAX_WALK_SPEED;

  switch (state) {
  case STATE_STAND:      controlStand    (mdx, jump, last_jump); break;
  case STATE_WALK:       controlWalk     (mdx, jump, last_jump); break;
  case STATE_JUMP_START: controlJumpStart(mdx, jump, last_jump); break;
  case STATE_JUMP_END:   controlJumpEnd  (mdx, jump, last_jump); break;
  }
}

void GameCharacter::move(GameJoy &joy)
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
      state = (joy.cur_x < 1000 || joy.cur_x > 3000) ? STATE_WALK : STATE_STAND;
      dy = 0;
      frame = 0;
    } else {           /* Hit the ceiling */
      dy = -dy;
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
