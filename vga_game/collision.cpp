/**
 * Most of this code is over 20 years old, written by confused young me.
 * It works, so lazy old me is too lazy to rewrite it. So here it is.
 */

#include "collision.h"
#include "game_data.h"

#ifndef ABS
#define ABS(x)       (((x) < 0) ? -(x) : (x))
#endif /* ABS */

#ifndef MAX
#define MAX(a,b)     (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a,b)     (((a) < (b)) ? (a) : (b))
#endif /* MIN */

struct RECT {
  int x, y;
  int w, h;
};

struct POINT {
  int x, y;
};

static int POINT_TO_MAP(int x, int y)
{
  if (x < 0 || y < 0 || x >= game_map.width * TILE_WIDTH || y >= game_map.height * TILE_HEIGHT)
    return MAP_BLOCK;
  return game_map.tiles[(y / TILE_HEIGHT) * game_map.width + x / TILE_WIDTH].block;
}

static int point_in_rect(int px, int py, int x, int y, int w, int h)
{
  return (px >= x && py >= y && px < x + w && py < y + h);
}

/* Return in `c' the interception of the rectangles `a' and `b' (if any).
 * If there's interception, returns 1; otherwise, returns 0 */
static int rect_interception(RECT *a, RECT *b, RECT *c)
{
  c->x = MAX(a->x, b->x);
  c->y = MAX(a->y, b->y);
  c->w = MIN(a->x + a->w, b->x + b->w) - c->x;
  c->h = MIN(a->y + a->h, b->y + b->h) - c->y;

  return (c->w > 0 && c->h > 0);
}

static RECT clip_block[][2] = {
  { { 0, 0,  2, 2 },  { -1 } },      /* Full */

  { { 0, 0,  1, 1 },  { -1 } },      /* Upper left */
  { { 1, 0,  1, 1 },  { -1 } },      /* Upper right */
  { { 0, 1,  1, 1 },  { -1 } },      /* Lower left */
  { { 1, 1,  1, 1 },  { -1 } },      /* Lower right */

  { { 0, 0,  2, 1 },  { -1 } },      /* Upper */
  { { 0, 0,  1, 2 },  { -1 } },      /* Left */
  { { 1, 0,  1, 2 },  { -1 } },      /* Right */
  { { 0, 1,  2, 1 },  { -1 } },      /* Lower */

  { { 1, 0,  1, 1 },  { 0, 1,  2, 1 } },      /* No upper left */
  { { 0, 0,  1, 1 },  { 0, 1,  2, 1 } },      /* No upper right */
  { { 0, 0,  2, 1 },  { 1, 1,  1, 1 } },      /* No lower left */
  { { 0, 0,  2, 1 },  { 0, 1,  1, 1 } },      /* No lower right */

  { { 0, 0,  1, 1 },  { 1, 1,  1, 1 } },      /* Left cross */
  { { 1, 0,  1, 1 },  { 0, 1,  1, 1 } },      /* Right cross */
};


/* Return the number of blocking */
static int get_block_rect(int x, int y, RECT *r)
{
  int tile;

  tile = POINT_TO_MAP(x, y);
  x = (x / TILE_WIDTH ) * TILE_WIDTH;
  y = (y / TILE_HEIGHT) * TILE_HEIGHT;

  if (tile >= 0 && tile <= 14) {
    r[0] = clip_block[tile][0];
    r[1] = clip_block[tile][1];
    if (r[0].x >= 0) {
      r[0].x = x + (r[0].x * TILE_WIDTH ) / 2;
      r[0].y = y + (r[0].y * TILE_HEIGHT) / 2;
      r[0].w = (r[0].w * TILE_WIDTH ) / 2 - 1;
      r[0].h = (r[0].h * TILE_HEIGHT) / 2 - 1;
    } else
      return 0;
    if (r[1].x >= 0) {
      r[1].x = x + (r[1].x * TILE_WIDTH ) / 2;
      r[1].y = y + (r[1].y * TILE_HEIGHT) / 2;
      r[1].w = (r[1].w * TILE_WIDTH ) / 2 - 1;
      r[1].h = (r[1].h * TILE_HEIGHT) / 2 - 1;
      return 2;
    }
    return 1;
  }
  return 0;
}


/* Do the clipping for a rectangle wanting to go from `initial' to `final'.
 * If `final' intercepts `block', then `final' is changed to the maximum
 * possible movement without interception */
static int clip_rect(RECT *initial, POINT *delta, RECT *block)
{
  RECT final, inter;

  final = *initial;
  final.x += delta->x;
  final.y += delta->y;

  if (rect_interception(&final, block, &inter)) {
    if (final.x > initial->x)
      delta->x -= inter.w;
    else if (final.x < initial->x)
      delta->x += inter.w;

    if (final.y > initial->y)
      delta->y -= inter.h;
    else if (final.y < initial->y)
      delta->y += inter.h;
    return 1;
  }

  return 0;
}

/* Do collision detection for the vertex `vertex' for the rectangle
 * `initial' moveing with the velocity vector `vel'. Returns the flags
 * containing CM_X_CLIPPED or CM_Y_CLIPPED. */
static int clip_block_vertex(POINT *vertex, RECT *initial, POINT *vel)
{
  RECT rect[4];
  POINT delta[4];
  int clipped = 0, i, n;

  /* Quick hack to avoid the "chicken bug" */
  if (vel->x + initial->x < 0)
    vel->x = -initial->x;
  if (vel->y + initial->y < 0)
    vel->y = -initial->y;

  if (vel->x != 0) {
    n = get_block_rect(vertex->x + vel->x, vertex->y, rect);
    for (i = 0; i < n; i++) {
      delta[i].x = vel->x;
      delta[i].y = 0;
      clipped |= clip_rect(initial, delta + i, rect + i);
    }
    for (i = 0; i < n; i++)
      if (ABS(vel->x) > ABS(delta[i].x))
        vel->x = delta[i].x;
  }

  if (vel->y != 0) {
    n = get_block_rect(vertex->x, vertex->y + vel->y, rect);
    for (i = 0; i < n; i++) {
      delta[i].x = 0;
      delta[i].y = vel->y;
      clipped |= clip_rect(initial, delta + i, rect + i);
    }
    for (i = 0; i < n; i++)
      if (ABS(vel->y) > ABS(delta[i].y))
        vel->y = delta[i].y;
  }

  if (! clipped && vel->x != 0 && vel->y != 0) {
    n = get_block_rect(vertex->x + vel->x, vertex->y + vel->y, rect);
    for (i = 0; i < n; i++) {
      delta[i] = *vel;
      clipped |= clip_rect(initial, delta + i, rect + i);
    }
    for (i = 0; i < n; i++)
      if (ABS(vel->x) > ABS(delta[i].x))
        vel->x = delta[i].x;
  }

  return clipped;
}


/* Calculate the movement of a jack.  Given the jack clipping rect
 * (x,y,w,h) and its speed (dx, dy), this function returns in
 * (*ret_dx, *ret_dy) the amount of pixels that the jack can move.
 * The return value contains the flags CM_X_CLIPPED or CM_Y_CLIPPED,
 * corresponding to blocking in the horizontal and vertical,
 * respectivelly. */
int calc_movement(int x, int y, int w, int h, int dx, int dy, int *ret_dx, int *ret_dy)
{
  RECT initial;
  POINT vertex[4], delta;
  int i;

  if (dx == 0 && dy == 0) {
    *ret_dx = *ret_dy = 0;
    return 0;
  }

  w++;
  h++;
  /* Build initial rectangle and 8 want-to-go rectangles */
  initial.x = x;
  initial.y = y;
  initial.w = w;
  initial.h = h;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w - 1;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h - 1;
  vertex[3].x = x + w - 1;
  vertex[3].y = y + h - 1;

  /* Clip */
  delta.x = dx;
  delta.y = dy;
  for (i = 0; i < 4; i++)
    clip_block_vertex(vertex + i, &initial, &delta);

  *ret_dx = delta.x;
  *ret_dy = delta.y;

  return (((ABS(delta.x) < ABS(dx)) ? CM_X_CLIPPED : 0) |
          ((ABS(delta.y) < ABS(dy)) ? CM_Y_CLIPPED : 0));
}

/* Return 1 if the rectangle (x,y)-(w,h) is blocked */
int is_map_blocked(int x, int y, int w, int h)
{
  RECT rect[4];
  int n_rects;
  POINT vertex[4];
  int i, j;

  if (x < 0 || y < 0)
    return 1;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h;
  vertex[3].x = x + w;
  vertex[3].y = y + h;

  /* Check map */
  for (i = 0; i < 4; i++) {
    n_rects = get_block_rect(vertex[i].x, vertex[i].y, rect);
    for (j = 0; j < n_rects; j++)
      if (point_in_rect(vertex[i].x, vertex[i].y, rect[j].x, rect[j].y, rect[j].w, rect[j].h))
        return 1;
  }

  return 0;
}
