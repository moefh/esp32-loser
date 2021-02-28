
#include "game_data.h"
#include "game_screen.h"

#pragma GCC optimize ("-O3")

float GameScreen::fpsCounter(int cur_millis) {
  //calculate fps (smooth)
  if (cur_millis > last_millis) {
    float fps = last_fps * 0.95f + (0.05f*1000.f) / (cur_millis - last_millis);
    last_fps = fps;
    return fps;
  } else {
    return 1000;
  }
}

// draw image line when x%4 == 0
void GameScreen::drawImageLine0(unsigned int *screen, const unsigned int *image, int image_width) {
  for (int x = 0; x < image_width/4; x++) {
    *screen++ = *image++;
  }
  switch (image_width%4) {
  case 0: break; // nothing to do
  case 1: *screen = (*screen&0xff00ffff) | (*image&0x00ff0000); break;
  case 2: *screen = (*screen&0x0000ffff) | (*image&0xffff0000); break;
  case 3: *screen = (*screen&0x0000ff00) | (*image&0xffff00ff); break;
  }
}

// draw image line when x%4 == 1
void GameScreen::drawImageLine1(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block) {
  unsigned int cur, old;

  // first block (3 pixels)
  cur = *image++;
  if (image_width >= 3 && ! skip_first_block) {
    image_width -= 3;
    *screen = (*screen & 0x00ff0000) | ((cur << 8) & 0xff00ff00) | ((cur >> 24) & 0x000000ff);
    screen++;
  }

  // middle blocks (4 pixels)
  for (int x = 0; x < image_width/4; x++) {
    old = cur;
    cur = *image++;
    *screen = ((old << 8) & 0x00ff0000) | ((cur << 8) & 0xff00ff00) | ((cur >> 24) & 0x000000ff);
    screen++;
  }

  if (image_width % 4 == 0) return;

  // last block (1-3 pixels)
  old = cur;
  cur = *image;
  switch (image_width%4) {
  case 1: *screen = (*screen & 0xff00ffff) | ((old << 8) & 0x00ff0000); break;
  case 2: *screen = (*screen & 0x0000ffff) | ((old << 8) & 0x00ff0000) | ((cur << 8) & 0xff000000); break;
  case 3: *screen = (*screen & 0x0000ff00) | ((old << 8) & 0x00ff0000) | ((cur << 8) & 0xff000000) | ((cur >> 24) & 0x000000ff); break;
  }
}

// draw image line when x%4 == 2
void GameScreen::drawImageLine2(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block) {
  unsigned int cur, old;

  // first block (2 pixels)
  cur = *image++;
  if (image_width >= 2 && ! skip_first_block) {
    image_width -= 2;
    *screen = (*screen & 0xffff0000) | ((cur >> 16) & 0x0000ffff);
    screen++;
  }

  // middle blocks (4 pixels)
  for (int x = 0; x < image_width/4; x++) {
    old = cur;
    cur = *image++;
    *screen = ((old << 16) & 0xffff0000) | ((cur >> 16) & 0x0000ffff);
    screen++;
  }

  if (image_width % 4 == 0) return;

  // last block (1-3 pixels)
  old = cur;
  cur = *image;
  switch (image_width%4) {
  case 1: *screen = (*screen & 0xff00ffff) | ((old << 16) & 0x00ff0000); break;
  case 2: *screen = (*screen & 0x0000ffff) | ((old << 16) & 0xffff0000); break;
  case 3: *screen = (*screen & 0x0000ff00) | ((old << 16) & 0xffff0000) | ((cur >> 16) & 0x000000ff); break;
  }
}

// draw image line when x%4 == 3
void GameScreen::drawImageLine3(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block) {
  unsigned int cur, old;

  // first block (1 pixel)
  cur = *image++;
  if (image_width >= 1 && ! skip_first_block) {
    image_width -= 1;
    *screen = (*screen & 0xffff00ff) | ((cur >> 8) & 0x0000ff00);
    screen++;
  }

  // middle blocks (4 pixels)
  for (int x = 0; x < image_width/4; x++) {
    old = cur;
    cur = *image++;
    *screen = ((old >> 8) & 0x00ff00ff) | ((old << 24) & 0xff000000) | ((cur >> 8) & 0x0000ff00);
    screen++;
  }

  if (image_width % 4 == 0) return;  // technically not needed

  // last block (1-3 pixels)
  old = cur;
  switch (image_width%4) {
  case 1: *screen = (*screen & 0xff00ffff) | ((old >> 8) & 0x00ff0000); break;
  case 2: *screen = (*screen & 0x0000ffff) | ((old >> 8) & 0x00ff0000) | ((old << 24) & 0xff000000); break;
  case 3: *screen = (*screen & 0x0000ff00) | ((old >> 8) & 0x00ff00ff) | ((old << 24) & 0xff000000); break;
  }
}

// draw transparent image line when x%4 == 0
void GameScreen::drawImageLineTr0(unsigned int *screen, const unsigned int *image, int image_width) {
  for (int x = 0; x < image_width/4; x++) {
    unsigned int mask = GET_4PIX_TRANSP_MASK(*image);
    if (mask == 0xffffffff) {
      *screen++ = *image++;
    } else {
      *screen = (*screen & ~mask) | (*image++ & mask);
      screen++;
    }
  }

  if (image_width % 4 == 0) return;

  // TODO: optimize by calculating mask for the used pixels only
  unsigned int mask = GET_4PIX_TRANSP_MASK(*image);
  switch (image_width%4) {
  case 1: *screen = (*screen & ((~mask) | 0xff00ffff)) | (*image & mask); break;
  case 2: *screen = (*screen & ((~mask) | 0x0000ffff)) | (*image & mask); break;
  case 3: *screen = (*screen & ((~mask) | 0x0000ff00)) | (*image & mask); break;
  }
}

// draw transparent image line when x%4 == 1
void GameScreen::drawImageLineTr1(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block) {
  unsigned int cur, old;

  // first block (3 pixels)
  cur = *image++;
  if (image_width >= 3 && ! skip_first_block) {
    image_width -= 3;
    unsigned int block = ((cur << 8) & 0xff00ff00) | ((cur >> 24) & 0x000000ff);
    unsigned int mask = GET_PIX1_TRANSP_MASK(block) | GET_PIX2_TRANSP_MASK(block) | GET_PIX3_TRANSP_MASK(block);
    *screen = (*screen & ~mask) | (block & mask);
    screen++;
  }

  // middle blocks (4 pixels)
  for (int x = 0; x < image_width/4; x++) {
    old = cur;
    cur = *image++;
    unsigned int block = ((old << 8) & 0x00ff0000) | ((cur << 8) & 0xff00ff00) | ((cur >> 24) & 0x000000ff);
    unsigned int mask = GET_4PIX_TRANSP_MASK(block);
    if (mask == 0xffffffff) {
      *screen++ = block;
    } else {
      *screen = (*screen & ~mask) | (block & mask);
      screen++;
    }
  }

  if (image_width % 4 == 0) return;

  // last block (1-3 pixels)
  old = cur;
  cur = *image;
  switch (image_width%4) {
  case 0: break; // nothing to do
  case 1:
    {
      unsigned int block = ((old << 8) & 0x00ff0000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  case 2:
    {
      unsigned int block = ((old << 8) & 0x00ff0000) | ((cur << 8) & 0xff000000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  case 3:
    {
      unsigned int block = ((old << 8) & 0x00ff0000) | ((cur << 8) & 0xff000000) | ((cur >> 24) & 0x000000ff);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block) | GET_PIX2_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  }
}

// draw transparent image line when x%4 == 2
void GameScreen::drawImageLineTr2(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block) {
  unsigned int cur, old;

  // first block (2 pixels)
  cur = *image++;
  if (image_width >= 2 && ! skip_first_block) {
    image_width -= 2;
    unsigned int block = ((cur >> 16) & 0x0000ffff);
    unsigned int mask = GET_PIX2_TRANSP_MASK(block) | GET_PIX3_TRANSP_MASK(block);
    *screen = (*screen & ~mask) | (block & mask);
    screen++;
  }

  // middle blocks (4 pixels)
  for (int x = 0; x < image_width/4; x++) {
    old = cur;
    cur = *image++;
    unsigned int block = ((old << 16) & 0xffff0000) | ((cur >> 16) & 0x0000ffff);
    unsigned int mask = GET_4PIX_TRANSP_MASK(block);
    if (mask == 0xffffffff) {
      *screen++ = block;
    } else {
      *screen = (*screen & ~mask) | (block & mask);
      screen++;
    }
  }

  if (image_width % 4 == 0) return;

  // last block (1-3 pixels)
  old = cur;
  cur = *image;
  switch (image_width%4) {
  case 1:
    {
      unsigned int block = ((old << 16) & 0x00ff0000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  case 2:
    {
      unsigned int block = ((old << 16) & 0xffff0000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  case 3:
    {
      unsigned int block = ((old << 16) & 0xffff0000) | ((cur >> 16) & 0x000000ff);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block) | GET_PIX2_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  }
}

// draw transparent image line when x%4 == 3
void GameScreen::drawImageLineTr3(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block) {
  unsigned int cur, old;

  // first block (1 pixel)
  cur = *image++;
  if (image_width >= 1 && ! skip_first_block) {
    image_width -= 1;
    unsigned int block = ((cur >> 8) & 0x0000ff00);
    unsigned int mask = GET_PIX3_TRANSP_MASK(block);
    *screen = (*screen & ~mask) | (block & mask);
    screen++;
  }

  // middle blocks (4 pixels)
  for (int x = 0; x < image_width/4; x++) {
    old = cur;
    cur = *image++;
    unsigned int block = ((old >> 8) & 0x00ff00ff) | ((old << 24) & 0xff000000) | ((cur >> 8) & 0x0000ff00);
    unsigned int mask = GET_4PIX_TRANSP_MASK(block);
    if (mask == 0xffffffff) {
      *screen++ = block;
    } else {
      *screen = (*screen & ~mask) | (block & mask);
      screen++;
    }
  }

  if (image_width % 4 == 0) return;  // technically not needed

  // last block (1-3 pixels)
  old = cur;
  switch (image_width%4) {
  case 1:
    {
      unsigned int block = ((old >> 8) & 0x00ff0000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  case 2:
    {
      unsigned int block = ((old >> 8) & 0x00ff0000) | ((old << 24) & 0xff000000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  case 3:
    {
      unsigned int block = ((old >> 8) & 0x00ff00ff) | ((old << 24) & 0xff000000);
      unsigned int mask = GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block) | GET_PIX2_TRANSP_MASK(block);
      *screen = (*screen & ~mask) | (block & mask);
    }
    break;
  }
}

void GameScreen::drawSprite(const SPRITE_DEF *def, int spr_x, int spr_y, int frame, bool transparent) {
  const unsigned int *image_start = &def->data[def->stride * def->height * frame];
  
  int height = def->height;
  if (spr_y < 0) {
    image_start += def->stride * (-spr_y);
    height += spr_y;
    spr_y = 0;
  }
  if (height > yres - spr_y) height = yres - spr_y;
  if (height <= 0) return;

  bool skip_first_block = false;
  int width = def->width;
  if (spr_x < 0) {
    image_start += (-spr_x) / 4;
    width += spr_x;
    spr_x = ((unsigned int) spr_x) % 4;
    skip_first_block = true;
  }
  if (width > xres - spr_x) width = xres - spr_x;
  if (width <= 0) return;

#define LINE(l) ((unsigned int *)backBuffer[l])
  if (transparent) {
    switch (spr_x % 4) {
    case 0: for (int y = 0; y < height; y++) drawImageLineTr0(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width); break;
    case 1: for (int y = 0; y < height; y++) drawImageLineTr1(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width, skip_first_block); break;
    case 2: for (int y = 0; y < height; y++) drawImageLineTr2(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width, skip_first_block); break;
    case 3: for (int y = 0; y < height; y++) drawImageLineTr3(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width, skip_first_block); break;
    }
  } else {
    switch (spr_x % 4) {
    case 0: for (int y = 0; y < height; y++) drawImageLine0(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width); break;
    case 1: for (int y = 0; y < height; y++) drawImageLine1(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width, skip_first_block); break;
    case 2: for (int y = 0; y < height; y++) drawImageLine2(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width, skip_first_block); break;
    case 3: for (int y = 0; y < height; y++) drawImageLine3(LINE(y+spr_y) + spr_x/4, image_start + def->stride*y, width, skip_first_block); break;
    }
  }
#undef LINE
}

void GameScreen::clearScreen(unsigned char color) {
  unsigned int clear_4pixels = (((SBits | (color&0x3f)) << 16) |
                                ((SBits | (color&0x3f)) << 24) |
                                ((SBits | (color&0x3f)) <<  0) |
                                ((SBits | (color&0x3f)) <<  8));
  for (int y = 0; y < yres; y++) {
    unsigned int *line = (unsigned int *) backBuffer[y];
    for (int x = 0; x < xres/4; x++) {
      line[x] = clear_4pixels;
    }
  }
}

void GameScreen::limitScreenPos() {
  if (game_data->screen_x < 0) {
    game_data->screen_x = 0;
  } else if (game_data->screen_x >= game_map.width*TILE_WIDTH - xres) {
    game_data->screen_x = game_map.width*TILE_WIDTH - xres - 1;
  }

  if (game_data->screen_y < 0) {
    game_data->screen_y = 0;
  } else if (game_data->screen_y >= game_map.height*TILE_HEIGHT - yres) {
    game_data->screen_y = game_map.height*TILE_HEIGHT - yres - 1;
  }
}

void GameScreen::setSprites(int num_sprites, SPRITE *sprites) {
  this->num_sprites = num_sprites;
  this->sprites = sprites;
  
  bool ok = true;
  for (int i = 0; i < num_sprites; i++) {
    if (! checkImageSBits(sprites[i].def->data)) {
      ok = false;
    }
  }
  setImagesSBitsOk(ok);
  
  if (! ok) {
    Serial.println("ERROR: bad SBits in sprite images");
    Serial.print("Current VGA mode SBits value: ");
    Serial.println(SBits);
  }
}

void GameScreen::renderScreen() {
  game_data->screen_x = (sprites[0].x-sprites[0].def->width/2 ) - xres/2;
  game_data->screen_y = (sprites[0].y-sprites[0].def->height/2) - yres/2;
  limitScreenPos();

  int tile_x_first = game_data->screen_x/TILE_WIDTH;
  int tile_x_last = (game_data->screen_x+xres)/TILE_WIDTH;
  int tile_y_first = game_data->screen_y/TILE_HEIGHT;
  int tile_y_last = (game_data->screen_y+yres)/TILE_HEIGHT;

  int x_pos_start = -(game_data->screen_x%TILE_WIDTH);
  int y_pos_start = -(game_data->screen_y%TILE_HEIGHT);

  int y_pos;

  // background
  y_pos = y_pos_start;
  for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
    int x_pos = x_pos_start;
    const MAP_TILE *tiles = &game_map.tiles[tile_y*game_map.width];
    for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
      int tile_num = tiles[tile_x].back;
      if (tile_num != 0xffff) {
        drawSprite(game_map.tileset, x_pos, y_pos, tile_num, false);
      }
      x_pos += TILE_WIDTH;
    }
    y_pos += TILE_HEIGHT;
  }

  // sprites
  for (int i = 0; i < num_sprites; i++) {
    int spr_x = sprites[i].x - game_data->screen_x;
    int spr_y = sprites[i].y - game_data->screen_y;
    if (spr_x <= -sprites[i].def->width) continue;
    if (spr_y <= -sprites[i].def->height) continue;
    if (spr_x >= xres || spr_y >= yres) continue;
    drawSprite(sprites[i].def, spr_x, spr_y, sprites[i].frame, true);
  }

  // foreground
  y_pos = y_pos_start;
  for (int tile_y = tile_y_first; tile_y <= tile_y_last; tile_y++) {
    int x_pos = x_pos_start;
    const MAP_TILE *tiles = &game_map.tiles[tile_y*game_map.width];
    for (int tile_x = tile_x_first; tile_x <= tile_x_last; tile_x++) {
      int tile_num = tiles[tile_x].fore;
      if (tile_num != 0xffff) {
        drawSprite(game_map.tileset, x_pos, y_pos, tile_num, true);
      }
      x_pos += TILE_WIDTH;
    }
    y_pos += TILE_HEIGHT;
  }
}

void GameScreen::show(int cur_millis) {
  if (imagesSBitsOk) {
    clearScreen(0x30);
    renderScreen();
  } else {
    clearScreen(0x30);
    setCursor(10, 10); print("Image data doesn't match");
    setCursor(10, 20); print("VGA mode SBits");
    setCursor(10, 30); print((int) SBits);
  }

  float fps = fpsCounter(cur_millis);
  last_millis = cur_millis;
  setCursor(250, 10);
  print("fps: ");
  print(fps, 1, 4);

  setCursor(10, 10); print("x"); setCursor(30, 10); print(sprites[0].x);
  setCursor(10, 20); print("y"); setCursor(30, 20); print(sprites[0].y);
  
  VGA6Bit::show();
}
