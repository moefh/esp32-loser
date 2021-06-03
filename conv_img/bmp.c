/* bmp.c - a .bmp file reader */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bmp.h"

static int f_read_data(FILE *f, void *data, size_t len)
{
  return (fread(data, 1, len, f) != len) ? -1 : 0;
}

static int f_set_pos(FILE *f, uint32_t offset)
{
  return (fseek(f, offset, SEEK_SET) < 0) ? -1 : 0;
}

static int f_read_u8(FILE *f)
{
  return fgetc(f);
}

static int32_t f_read_u16(FILE *f)
{
  unsigned char b[2];
  if (f_read_data(f, b, sizeof(b)) < 0) {
    return -1;
  }
  return ((((uint16_t) b[0]) << 0) |
          (((uint16_t) b[1]) << 1));
}

static int64_t f_read_u32(FILE *f)
{
  unsigned char b[4];
  if (f_read_data(f, b, sizeof(b)) < 0) {
    return -1;
  }
  return ((((uint32_t) b[0]) <<  0) |
          (((uint32_t) b[1]) <<  8) |
          (((uint32_t) b[2]) << 16) |
          (((uint32_t) b[3]) << 24));
}

static int get_shift_for_mask(uint32_t mask)
{
  if (mask == 0) return 0;
  
  int shift = 0;
  while ((mask & 1) == 0) {
    mask >>= 1;
    shift++;
  }
  return shift;
}

struct BMP_IMAGE *bmp_read(const char *filename, int color_format)
{
  struct BMP_IMAGE *bmp = NULL;
  FILE *f = fopen(filename, "rb");
  if (! f) {
    return NULL;
  }

  // magic
  unsigned char magic[2];
  if (f_read_data(f, magic, 2) < 0) goto error;
  if (magic[0] != 0x42 || magic[1] != 0x4D) goto error;

  if (f_read_u32(f) < 0) goto error;  // file size
  if (f_read_u16(f) < 0) goto error;  // reserved
  if (f_read_u16(f) < 0) goto error;  // reserved

  // data offset
  int64_t data_offset = f_read_u32(f);
  if (data_offset < 0) goto error;

  // DIB header
  int64_t dib_size = f_read_u32(f);
  if (dib_size < 40) goto error;
  int64_t file_w = f_read_u32(f); if (file_w < 0) goto error;    // image width
  int64_t file_h = f_read_u32(f); if (file_h < 0) goto error;    // image height
  if (f_read_u16(f) != 1) goto error;                            // planes (must be 1)
  int32_t bmp_bpp = f_read_u16(f); if (bmp_bpp < 0) goto error;  // bits per pixel

  // compression (we only accept none)
  bool read_bitfields = false;
  int64_t compression = f_read_u32(f);
  if (compression < 0) goto error;
  if (compression == 3 && dib_size >= 52) {
    read_bitfields = true;
    compression = 0;
  }
  if (compression != 0) {
    goto error;
  }

  int component_shift_r = 16;
  int component_shift_g = 8;
  int component_shift_b = 0;
  int component_shift_a = 24;
  if (read_bitfields) {
    if (dib_size < 52) goto error;
    if (f_set_pos(f, 0x36) < 0) goto error;
    int64_t mask;
    if ((mask = f_read_u32(f)) < 0) { goto error; }   component_shift_r = get_shift_for_mask(mask);
    if ((mask = f_read_u32(f)) < 0) { goto error; }   component_shift_g = get_shift_for_mask(mask);
    if ((mask = f_read_u32(f)) < 0) { goto error; }   component_shift_b = get_shift_for_mask(mask);
    if (dib_size >= 56) {
      if ((mask = f_read_u32(f)) < 0) { goto error; } component_shift_a = get_shift_for_mask(mask);
    }
  }

  // calculate sizes
  int32_t bmp_w = (int32_t) file_w;
  int32_t bmp_h = (int32_t) file_h;
  bool flip = true;
  if (bmp_h < 0) {
    bmp_h = -bmp_h;
    flip = false;
  }
  size_t stride = ((bmp_bpp * bmp_w + 31) / 32) * 4;
  
  // data
  if (f_set_pos(f, data_offset) < 0) goto error;
  bmp = malloc(sizeof(struct BMP_IMAGE) + bmp_h * stride);
  if (! bmp) goto error;
  for (int y = 0; y < bmp_h; y++) {
    unsigned char *line = (flip) ? &bmp->data[(bmp_h-1-y)*stride] : &bmp->data[y*stride];
    switch (bmp_bpp) {
    case 32:
      for (int x = 0; x < bmp_w; x++) {
        int64_t pixel = f_read_u32(f);
        if (pixel < 0) goto error;
        switch (color_format) {
        case BMP_FORMAT_BGRA:
          line[4*x+0] = ((uint32_t) pixel) >> component_shift_b;
          line[4*x+1] = ((uint32_t) pixel) >> component_shift_g;
          line[4*x+2] = ((uint32_t) pixel) >> component_shift_r;
          line[4*x+3] = ((uint32_t) pixel) >> component_shift_a;
          break;

        case BMP_FORMAT_RGBA:
          line[4*x+0] = ((uint32_t) pixel) >> component_shift_r;
          line[4*x+1] = ((uint32_t) pixel) >> component_shift_g;
          line[4*x+2] = ((uint32_t) pixel) >> component_shift_b;
          line[4*x+3] = ((uint32_t) pixel) >> component_shift_a;
          break;

        default:
          line[4*x+0] = ((uint32_t) pixel) >> 0;
          line[4*x+1] = ((uint32_t) pixel) >> 8;
          line[4*x+2] = ((uint32_t) pixel) >> 16;
          line[4*x+3] = ((uint32_t) pixel) >> 24;
          break;
        }
      }
      break;

    case 24:
      for (int x = 0; x < bmp_w; x++) {
        switch (color_format) {
        case BMP_FORMAT_RGBA:
          for (int c = 0; c < 3; c++) {
            int b = f_read_u8(f);
            if (b < 0) goto error;
            line[3*x+2-c] = b;
          }
          break;

        case BMP_FORMAT_BGRA:
          for (int c = 0; c < 3; c++) {
            int b = f_read_u8(f);
            if (b < 0) goto error;
            line[3*x+c] = b;
          }
          break;

        case BMP_FORMAT_FILE:
          for (int c = 0; c < 3; c++) {
            int b = f_read_u8(f);
            if (b < 0) goto error;
            line[3*x+c] = b;
          }
          break;
        } 
      }
      for (int pad = 0; pad < stride - 3*bmp_w; pad++) {
        if (f_read_u8(f) < 0) goto error;
      }
      break;

    default:
      if (f_read_data(f, line, stride) < 0) goto error;
      break;
    }
  }

  // bmp data
  bmp->w = bmp_w;
  bmp->h = bmp_h;
  bmp->bpp = bmp_bpp;
  bmp->stride = stride;
  fclose(f);
  return bmp;

 error:
  bmp_free(bmp);
  fclose(f);
  return NULL;
}

void bmp_free(struct BMP_IMAGE *bmp)
{
  free(bmp);
}

