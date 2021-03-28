
#include <Arduino.h>

#include "font.h"

void draw_text(uint8_t **framebuffer, uint8_t sync_bits, const FONT &font, unsigned int x, unsigned int y, uint8_t color, int num)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", num);
  draw_text(framebuffer, sync_bits, font, x, y, color, buf);
}

void draw_text(uint8_t **framebuffer, uint8_t sync_bits, const FONT &font, unsigned int x, unsigned int y, uint8_t color, float num)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%f", num);
  draw_text(framebuffer, sync_bits, font, x, y, color, buf);
}

void draw_text(uint8_t **framebuffer, uint8_t sync_bits, const FONT &font, unsigned int x, unsigned int y, uint8_t color, const char *text)
{
  while (*text != '\0') {
    char ch = *text++;
    if (ch >= font.first_char && ch < font.first_char+font.num_chars) {
      ch -= font.first_char;
      for (int i = 0; i < font.h; i++) {
        uint8_t char_line = font.data[font.h*ch + i];
        uint8_t char_bit = 1;
        for (int j = 0; j < font.w; j++) {
          if ((char_line & char_bit) != 0 && y+i < 240 && x+j < 320) {
            framebuffer[y+i][(x+j)^2] = sync_bits | (color & 0x3f);
          }
          char_bit <<= 1;
        }
      }
    }
    x += font.w;
  }
}
