
#include "game_data.h"

// maps
#include "map.h"

// sprites
#include "spr_castle3.h"
#include "spr_loserboy.h"
#include "spr_punkman.h"
#include "spr_stickman.h"
#include "spr_powerup.h"
#include "spr_blacknight.h"

GAME_DATA game_data;

const SPRITE_DEF game_sprite_defs[] = {
#define ADD_SPRITE_DEF(name) { img_##name##_width, img_##name##_height, img_##name##_stride, img_##name##_num_spr, img_##name##_data }
  ADD_SPRITE_DEF(castle3),
  ADD_SPRITE_DEF(powerup),
  ADD_SPRITE_DEF(loserboy),
  ADD_SPRITE_DEF(punkman),
  ADD_SPRITE_DEF(stickman),
  ADD_SPRITE_DEF(blacknight),
};
const int game_num_sprite_defs = (int) (sizeof(game_sprite_defs)/sizeof(*game_sprite_defs));

SPRITE game_sprites[] = {
  { &game_sprite_defs[2],  64,  64,  0, },   // loserboy
  { &game_sprite_defs[1], 204,  10,  0, },   // powerup
  { &game_sprite_defs[3], 384, 512,  0, },   // punkman
  { &game_sprite_defs[4], 248, 142, 12, },   // stickman
  { &game_sprite_defs[5], 448,  70,  6, },   // blacknight
};
const int game_num_sprites = (int) (sizeof(game_sprites)/sizeof(*game_sprites));

const CHAR_DEF char_def = {
  .clip = { 15, 5, 31, 35 },
  .mirror = 11,
  .shoot_frame = 22,
  .num_stand = 1,
  .stand = { 10 },
  .num_jump = 1,
  .jump = { 4 },
  .num_walk = 36,
  .walk = {
    5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5,
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0,
  },
};
