#ifndef GAME_SCREEN_H_FILE
#define GAME_SCREEN_H_FILE

/**
 * The GameScreen class handles the game screen. It writes directly to
 * vga_6bit's freamebuffer.
 */

#include "game_data.h"
#include "game_network.h"
#include "game_joy.h"

class GameScreen {
private:
  int last_millis = 0;
  int last_fps = 0;
  int fps_frame_count = 0;
  bool images_sbits_ok = false;
  unsigned char sync_bits;
  GameNetwork *net;
  GameJoy *joy;

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

  void init(const int *pin_config, GameNetwork *net, GameJoy *joy);

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
