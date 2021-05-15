
#ifndef VGA_FONT_H_FILE
#define VGA_FONT_H_FILE

#include <cstdint>

#define FONT_CURSOR ((unsigned int) (-1))

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
  const FONT *font;
} FONT_INFO;

void font_set_cursor(unsigned int x, unsigned int y);
void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, int num);
void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, unsigned int num);
void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, float num);
void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, const char *text);

void font_draw(const FONT_INFO &info, uint8_t color, int num);
void font_draw(const FONT_INFO &info, uint8_t color, unsigned int num);
void font_draw(const FONT_INFO &info, uint8_t color, float num);
void font_draw(const FONT_INFO &info, uint8_t color, const char *text);

#endif /* VGA_FONT_H_FILE */
