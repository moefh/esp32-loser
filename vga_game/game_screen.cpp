#include <cstring>

#include "game_screen.h"
#include "game_data.h"

#include "util.h"
#include "vga_6bit.h"
#include "vga_font.h"
#include "font6x8.h"

#pragma GCC optimize ("-O3")

enum {
  DEBUG_SHOW_NOTHING,
  DEBUG_SHOW_FPS,
  DEBUG_SHOW_POSITION,
  DEBUG_SHOW_NETWORK,
  DEBUG_SHOW_CONTROLLER,
  DEBUG_SHOW_FRAMETIME,
  DEBUG_MAX_LEVEL
};

// x-coord : |        0        1        2        3 |        4        5        6        7 | ...
// address : |      [2]      [3]      [0]      [1] |      [6]      [7]      [4]      [5] | ...
// mask    : | 00ff0000 ff000000 000000ff 0000ff00 | 00ff0000 ff000000 000000ff 0000ff00 | ...

#define GET_PIX0_TRANSP_MASK(block) ((((block) & 0x003f0000) != 0x000c0000) ? 0x00ff0000 : 0)
#define GET_PIX1_TRANSP_MASK(block) ((((block) & 0x3f000000) != 0x0c000000) ? 0xff000000 : 0)
#define GET_PIX2_TRANSP_MASK(block) ((((block) & 0x0000003f) != 0x0000000c) ? 0x000000ff : 0)
#define GET_PIX3_TRANSP_MASK(block) ((((block) & 0x00003f00) != 0x00000c00) ? 0x0000ff00 : 0)
#define GET_4PIX_TRANSP_MASK(block) (GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block) | \
                                     GET_PIX2_TRANSP_MASK(block) | GET_PIX3_TRANSP_MASK(block))

// Select screen resolution according to the enabled features (there's
// not enough memory to enable everything at max resolution)
static const VgaMode &get_vga_mode(GameNetwork *net, GameJoy *joy)
{
  if (net->is_running() && joy->getType() == CONTROLLER_WIIMOTE) {
    // WiFi with Bluetooth:
    return vga_mode_240x240;
  }
    
  // no WiFi or WiFi without Bluetooth:
  return vga_mode_320x240;
}


void GameScreen::init(const int *pin_config, GameNetwork *net, GameJoy *joy) {
  vga_init(pin_config, get_vga_mode(net, joy));
  screen_w  = vga_get_xres();
  screen_h  = vga_get_yres();
  sync_bits = vga_get_sync_bits();
  debug_level = DEBUG_MAX_LEVEL;
  last_btn_press_frame = 0;
  checkSprites();
  this->net = net;
  this->joy = joy;
}

void GameScreen::checkSprites() {
  bool ok = true;
  for (int i = 0; i < game_num_sprite_defs; i++) {
    if (! checkImageSBits(game_sprite_defs[i].data)) {
      ok = false;
    }
  }
  setImagesSBitsOk(ok);
  
  if (! ok) {
    printf("ERROR: bad SBits in sprite images\n");
    printf("Current VGA mode SBits value: %02x\n", sync_bits);
  }
}

int GameScreen::fpsCounter(int cur_millis) {
  if (cur_millis/1000 != last_millis/1000) {
    last_fps = fps_frame_count;
    fps_frame_count = 1;
  } else {
    fps_frame_count++;
  }
  return last_fps;
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
  if (height > screen_h - spr_y) height = screen_h - spr_y;
  if (height <= 0) return;

  bool skip_first_block = false;
  int width = def->width;
  if (spr_x < 0) {
    image_start += (-spr_x) / 4;
    width += spr_x;
    spr_x = ((unsigned int) spr_x) % 4;
    skip_first_block = true;
  }
  if (width > screen_w - spr_x) width = screen_w - spr_x;
  if (width <= 0) return;

  unsigned char **framebuffer = vga_get_framebuffer();
#define LINE(l) ((unsigned int *)framebuffer[l])
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

void GameScreen::setScreenPos() {
  screen_x = game_data.camera_x - screen_w/2;
  screen_y = game_data.camera_y - screen_h/2;

  if (screen_x < 0) {
    screen_x = 0;
  } else if (screen_x >= game_map.width*TILE_WIDTH - screen_w) {
    screen_x = game_map.width*TILE_WIDTH - screen_w - 1;
  }

  if (screen_y < 0) {
    screen_y = 0;
  } else if (screen_y >= game_map.height*TILE_HEIGHT - screen_h) {
    screen_y = game_map.height*TILE_HEIGHT - screen_h - 1;
  }
}

void GameScreen::renderScreen() {
  setScreenPos();

  int tile_x_first = screen_x/TILE_WIDTH;
  int tile_x_last = (screen_x+screen_w)/TILE_WIDTH;
  int tile_y_first = screen_y/TILE_HEIGHT;
  int tile_y_last = (screen_y+screen_h)/TILE_HEIGHT;

  int x_pos_start = -(screen_x%TILE_WIDTH);
  int y_pos_start = -(screen_y%TILE_HEIGHT);

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
  for (int i = 0; i < GAME_NUM_SPRITES; i++) {
    if (! game_sprites[i].def) continue;
    int spr_x = game_sprites[i].x - screen_x;
    int spr_y = game_sprites[i].y - screen_y;
    if (spr_x <= -game_sprites[i].def->width) continue;
    if (spr_y <= -game_sprites[i].def->height) continue;
    if (spr_x >= screen_w || spr_y >= screen_h) continue;
    drawSprite(game_sprites[i].def, spr_x, spr_y, game_sprites[i].frame, true);
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

void GameScreen::renderDebugInfo(FONT_INFO &fi, int cur_millis) {
  int fps = fpsCounter(cur_millis);
  last_millis = cur_millis;

  if ((! net->is_running()) && (joy->cur & JOY_BTN_E)) {
    net->init();
  }

  if (JOY_BTN_PRESSED(joy, JOY_BTN_F) && (frame_count-last_btn_press_frame > 5)) {
    debug_level++;
    if (debug_level >= DEBUG_MAX_LEVEL-1) {
      debug_level = 0;
    }
    last_btn_press_frame = frame_count;
  }
  
  if (debug_level >= DEBUG_SHOW_FPS) {
    font_set_cursor(10, 10);
    font_draw(fi, 0x3f, fps);
    font_draw(fi, 0x3f, " fps");
  }

  if (debug_level >= DEBUG_SHOW_POSITION) {
    font_draw(fi, screen_w-46, 10, 0x3f, "x "); font_draw(fi, 0x3f, game_sprites[0].x);
    font_draw(fi, screen_w-46, 20, 0x3f, "y "); font_draw(fi, 0x3f, game_sprites[0].y);
    if (net->is_running()) {
      font_draw(fi, screen_w-46, 30, 0x3f, "x "); font_draw(fi, 0x3f, game_sprites[1].x);
      font_draw(fi, screen_w-46, 40, 0x3f, "y "); font_draw(fi, 0x3f, game_sprites[1].y);
    }
  }

  if (debug_level >= DEBUG_SHOW_NETWORK) {
    if (net->is_running()) {
      font_set_cursor(10, screen_h-20);
      font_draw(fi, 0x3f, "Network enabled: ");
      font_draw(fi, 0x3f, net->get_num_tx_packets());
      font_draw(fi, 0x3f, ":");
      font_draw(fi, 0x3f, net->get_num_tx_errors());
    } else {
      font_draw(fi, 10, screen_h-20, 0x3f, "Network disabled");
    }
  }
  
  if (debug_level >= DEBUG_SHOW_CONTROLLER) {
    int len = strlen(joy->getName()) + 12;
    font_set_cursor(screen_w-6*len-10, screen_h-20);
    font_draw(fi, 0x3f, "Controller: ");
    font_draw(fi, 0x3f, joy->getName());

    char btns_pressed[11];
    char *p = btns_pressed;
    if (joy->cur & JOY_BTN_A    ) *p++ = 'A';
    if (joy->cur & JOY_BTN_B    ) *p++ = 'B';
    if (joy->cur & JOY_BTN_C    ) *p++ = 'C';
    if (joy->cur & JOY_BTN_D    ) *p++ = 'D';
    if (joy->cur & JOY_BTN_E    ) *p++ = 'E';
    if (joy->cur & JOY_BTN_F    ) *p++ = 'F';
    if (joy->cur & JOY_BTN_LEFT ) *p++ = '<';
    if (joy->cur & JOY_BTN_RIGHT) *p++ = '>';
    if (joy->cur & JOY_BTN_UP   ) *p++ = '^';
    if (joy->cur & JOY_BTN_DOWN ) *p++ = 'v';
    *p = '\0';
    font_draw(fi, screen_w-70, screen_h-30, 0x3f, btns_pressed);
  }
}

void GameScreen::clear(unsigned char color) {
  vga_clear_screen(color);
}

void GameScreen::show(int cur_millis) {
  FONT_INFO fi = { screen_w, screen_h, sync_bits, vga_get_framebuffer(), &font6x8 };

  if (images_sbits_ok) {
    renderScreen();
  } else {
    clear(0x30);
    font_draw(fi, 10, 40, 0x3f, "Image data doesn't match");
    font_draw(fi, 10, 50, 0x3f, "VGA mode sync bits");
    font_draw(fi, 10, 60, 0x3f, (int) sync_bits);
  }

  renderDebugInfo(fi, cur_millis);
  
  frame_count++;
  vga_swap_buffers();
}
