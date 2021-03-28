
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

void draw_text(uint8_t **framebuffer, uint8_t sync_bits, const FONT &font, unsigned int x, unsigned int y, uint8_t color, int num);
void draw_text(uint8_t **framebuffer, uint8_t sync_bits, const FONT &font, unsigned int x, unsigned int y, uint8_t color, float num);
void draw_text(uint8_t **framebuffer, uint8_t sync_bits, const FONT &font, unsigned int x, unsigned int y, uint8_t color, const char *text);

#endif /* VGA_FONT_H_FILE */
