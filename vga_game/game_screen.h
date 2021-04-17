#ifndef GAME_SCREEN_H_FILE
#define GAME_SCREEN_H_FILE

/**
 * The GameScreen class handles the game screen. It writes directly to
 * vga_6bit's freamebuffer.
 */

#include "game_data.h"

// x-coord : |        0        1        2        3 |        4        5        6        7 | ...
// address : |      [2]      [3]      [0]      [1] |      [6]      [7]      [4]      [5] | ...
// mask    : | 00ff0000 ff000000 000000ff 0000ff00 | 00ff0000 ff000000 000000ff 0000ff00 | ...

#define GET_PIX0_TRANSP_MASK(block) ((((block) & 0x003f0000) != 0x000c0000) ? 0x00ff0000 : 0)
#define GET_PIX1_TRANSP_MASK(block) ((((block) & 0x3f000000) != 0x0c000000) ? 0xff000000 : 0)
#define GET_PIX2_TRANSP_MASK(block) ((((block) & 0x0000003f) != 0x0000000c) ? 0x000000ff : 0)
#define GET_PIX3_TRANSP_MASK(block) ((((block) & 0x00003f00) != 0x00000c00) ? 0x0000ff00 : 0)
#define GET_4PIX_TRANSP_MASK(block) (GET_PIX0_TRANSP_MASK(block) | GET_PIX1_TRANSP_MASK(block) | \
                                     GET_PIX2_TRANSP_MASK(block) | GET_PIX3_TRANSP_MASK(block))

class GameScreen {
private:
  int last_millis = 0;
  int last_fps = 0;
  int fps_frame_count = 0;
  bool images_sbits_ok = false;
  unsigned char sync_bits;

  int screen_x;
  int screen_y;
  int screen_w;
  int screen_h;
  
protected:
  
  int fpsCounter(int cur_millis);

  void drawImageLine0(unsigned int *screen, const unsigned int *image, int image_width);
  void drawImageLine1(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block);
  void drawImageLine2(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block);
  void drawImageLine3(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block);
  
  void drawImageLineTr0(unsigned int *screen, const unsigned int *image, int image_width);
  void drawImageLineTr1(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block);
  void drawImageLineTr2(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block);
  void drawImageLineTr3(unsigned int *screen, const unsigned int *image, int image_width, bool skip_first_block);
  
  void renderScreen();
  void checkSprites();
  
public:

  void init(const int *pin_config, bool low_res_mode);
  void setImagesSBitsOk(bool ok) { images_sbits_ok = ok; }
  bool getImagesSBitsOk(bool ok) { return images_sbits_ok; }
  unsigned char getSBits() { return sync_bits; }

  bool checkImageSBits(const unsigned int *image_data) {
    return (sync_bits == (image_data[0]&0xc0));
  }

  void clear(unsigned char color = 0);
  void drawSprite(const SPRITE_DEF *def, int spr_x, int spr_y, int frame, bool transparent);
  void setScreenPos();
  void show(int cur_millis);
};

#endif /* GAME_SCREEN_H_FILE */
