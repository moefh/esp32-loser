/* bmp.h - a BMP file reader */

#ifndef BMP_H_FILE
#define BMP_H_FILE

#define BMP_FORMAT_BGRA  0    /* blue in the first byte */
#define BMP_FORMAT_RGBA  1    /* red  in the first byte */
#define BMP_FORMAT_FILE  2    /* use however it came from the file */

struct BMP_IMAGE {
  unsigned int w;
  unsigned int h;
  unsigned int bpp;
  unsigned int stride;
  unsigned char data[];
};

struct BMP_IMAGE *bmp_read(const char *filename, int color_format);
void bmp_free(struct BMP_IMAGE *bmp);

#endif /* BMP_H_FILE */
