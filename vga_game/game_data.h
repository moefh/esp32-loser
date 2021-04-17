#ifndef GAME_DATA_H_FILE
#define GAME_DATA_H_FILE

#define TILE_WIDTH  64
#define TILE_HEIGHT 64
#define TILE_STRIDE 16

#define GAME_NUM_SPRITE_DEF_SHOT    2

#define GAME_NUM_SPRITES            16
#define GAME_NUM_SPRITE_FIRST_SHOT  2

enum {
  MAP_BLOCK,
  MAP_BLOCK1,
  MAP_BLOCK2,
  MAP_BLOCK3,
  MAP_BLOCK4,
  MAP_BLOCK5,
  MAP_BLOCK6,
  MAP_BLOCK7,
  MAP_BLOCK8,
  MAP_BLOCK9,
  MAP_BLOCK10,
  MAP_BLOCK11,
  MAP_BLOCK12,
  MAP_BLOCK13,
  MAP_BLOCK14,

  MAP_SECRET,

  MAP_STARTRIGHT,
  MAP_STARTLEFT,

  N_MAP_BLOCKS,
};

struct SPRITE_DEF {
  int width;
  int height;
  int stride;
  int num_frames;
  const unsigned int *data;
};

struct MAP_TILE {
  unsigned short back;
  unsigned short fore;
  unsigned short block;
};

struct MAP_POINT {
  unsigned int x;     // fixed 16.16
  unsigned int y;     // fixed 16.16
};

struct MAP_SPAWN_POINT {
  MAP_POINT pos;
  unsigned char dir;  // 0=left, 1=right
};

struct MAP {
  int width;
  int height;
  const MAP_TILE *tiles;
  int num_spawn_points;
  const MAP_SPAWN_POINT *spawn_points;
  const SPRITE_DEF *tileset;
};

struct SPRITE {
  const SPRITE_DEF *def;
  int x;
  int y;
  int frame;
};

struct GAME_DATA {
  int camera_x;
  int camera_y;
};

struct CHAR_DEF {
  struct {
    unsigned char x;
    unsigned char y;
    unsigned char width;
    unsigned char height;
  } clip;
  unsigned int mirror;
  unsigned int shoot_frame;
  unsigned int num_stand;
  unsigned char stand[64];
  unsigned int num_jump;
  unsigned char jump[64];
  unsigned int num_walk;
  unsigned char walk[64];
};

extern const int game_num_sprite_defs;
extern const SPRITE_DEF game_sprite_defs[];

extern SPRITE game_sprites[];

extern const MAP game_map;
extern GAME_DATA game_data;

extern const CHAR_DEF char_def;

#endif /* GAME_DATA_H_FILE */
