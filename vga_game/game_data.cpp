
#include "game_data.h"

// maps
#include "map.h"

// sprites
#include "spr_castle3.h"
#include "spr_loserboy.h"
#include "spr_pwr2.h"

GAME_DATA game_data;

const SPRITE_DEF game_sprite_defs[] = {
#define ADD_SPRITE_DEF(name) { img_##name##_width, img_##name##_height, img_##name##_stride, img_##name##_num_spr, img_##name##_data }
  ADD_SPRITE_DEF(castle3),
  ADD_SPRITE_DEF(loserboy),
  ADD_SPRITE_DEF(pwr2),
};
const int game_num_sprite_defs = (int) (sizeof(game_sprite_defs)/sizeof(*game_sprite_defs));

SPRITE game_sprites[GAME_NUM_SPRITES] = {
  { &game_sprite_defs[1],  64,  64,  0, },   // loserboy (player)
  { &game_sprite_defs[1], -64, -64,  0, },   // loserboy (network)
};

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
