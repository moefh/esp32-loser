
#ifndef VGA_FONT_H_FILE
#define VGA_FONT_H_FILE

#include <cstdint>

typedef struct {
  int w;
  int h;
  int first_char;
  int num_chars;
  const uint8_t *data;
} FONT;

typedef struct {
  int x_res;
  int y_res;
  uint8_t sync_bits;
  uint8_t **framebuffer;
} FONT_SCREEN;

void draw_text(const FONT_SCREEN &screen, const FONT &font, unsigned int x, unsigned int y, uint8_t color, int num);
void draw_text(const FONT_SCREEN &screen, const FONT &font, unsigned int x, unsigned int y, uint8_t color, float num);
void draw_text(const FONT_SCREEN &screen, const FONT &font, unsigned int x, unsigned int y, uint8_t color, const char *text);

#endif /* VGA_FONT_H_FILE */
