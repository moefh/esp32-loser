
#include <stdio.h>

#include "vga_font.h"

static unsigned int font_x, font_y;

void font_set_cursor(unsigned int x, unsigned int y)
{
  font_x = x;
  font_y = y;
}

void font_draw(const FONT_INFO &info, uint8_t color, int num) {
  font_draw(info, FONT_CURSOR, FONT_CURSOR, color, num);
}
  
void font_draw(const FONT_INFO &info, uint8_t color, unsigned int num) {
  font_draw(info, FONT_CURSOR, FONT_CURSOR, color, num);
}

void font_draw(const FONT_INFO &info, uint8_t color, float num) {
  font_draw(info, FONT_CURSOR, FONT_CURSOR, color, num);
}

void font_draw(const FONT_INFO &info, uint8_t color, const char *text) {
  font_draw(info, FONT_CURSOR, FONT_CURSOR, color, text);
}

void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, int num)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", num);
  font_draw(info, x, y, color, buf);
}

void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, unsigned int num)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%u", num);
  font_draw(info, x, y, color, buf);
}

void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, float num)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%f", num);
  font_draw(info, x, y, color, buf);
}

void font_draw(const FONT_INFO &info, unsigned int x, unsigned int y, uint8_t color, const char *text)
{
  if (x == FONT_CURSOR) x = font_x;
  if (y == FONT_CURSOR) y = font_y;
  const FONT &font = *info.font;
  while (*text != '\0') {
    char ch = *text++;
    if (ch >= font.first_char && ch < font.first_char+font.num_chars) {
      ch -= font.first_char;
      for (int i = 0; i < font.h; i++) {
        uint8_t char_line = font.data[font.h*ch + i];
        uint8_t char_bit = 1;
        for (int j = 0; j < font.w; j++) {
          if ((char_line & char_bit) != 0 && y+i < info.y_res && x+j < info.x_res) {
            info.framebuffer[y+i][(x+j)^2] = info.sync_bits | (color & 0x3f);
          }
          char_bit <<= 1;
        }
      }
    }
    x += font.w;
  }
  font_y = y;
  font_x = x;
}
