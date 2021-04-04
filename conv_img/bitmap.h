/* bitmap.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef BITMAP_H_FILE
#define BITMAP_H_FILE

typedef unsigned short int2bpp;     /* Must be a 2 byte (16 bits) integer */
typedef unsigned int   int4bpp;     /* Must be a 4 byte (32 bits) integer */

#define MAKE_MAGIC(a,b,c,d)   (((d)<<24) | ((c)<<16) | ((b)<<8) | ((a)))

#define XBMP_MAGIC     MAKE_MAGIC(0xde,0xad,0xbe,0xef)
#define XBMP16_MAGIC   MAKE_MAGIC(0xde,0xad,0xf0,0x0d)
#define FONT_MAGIC     MAKE_MAGIC('f','n','8',' ')
#define FONT16_MAGIC   MAKE_MAGIC('f','n','1','6')

#if 0
#define MAKECOLOR8(r,g,b)			\
  ((r & 0xe0) | (g & 0x1c) | (b & 0x02))
#endif

#define MAKECOLOR16_real(r,g,b)			\
  ((int2bpp)					\
   (((((r)>>3) & 0x1f) << 11) |			\
    ((((g)>>2) & 0x3f) << 5 ) |			\
    ((((b)>>3) & 0x1f))))

#define MAKECOLOR15_real(r,g,b)			\
  ((int2bpp)					\
   (((((r)>>3) & 0x1f) << 10) |			\
    ((((g)>>3) & 0x1f) << 5 ) |			\
    ((((b)>>3) & 0x1f))))

#define MAKECOLOR16(r,g,b)  ((convert_16bpp_to == 15)	\
			     ? MAKECOLOR15_real(r,g,b)	\
			     : MAKECOLOR16_real(r,g,b))

#define MAKECOLOR32(r,g,b)			\
  ((int4bpp)					\
   ((((unsigned long)(r))<<16) |		\
    (((unsigned long)(g))<<8) |			\
    (((unsigned long)(b)))))


#define XBMP8_TRANSP      0    /* 0x1c in the uniform palette */
#if 0
#define XBMP15_TRANSP     MAKECOLOR15_real(0,0xff,0)
#else
#define XBMP15_TRANSP     0x3e0
#endif

#if (defined GRAPH_DOS)
#define FILE16_TRANSP     MAKECOLOR16_real(0,0xff,0)
#define XBMP16_TRANSP     MASK_COLOR_16
#define XBMP16_JACKCOLOR  MAKECOLOR16_real(0,0xff,0)
#elif (defined GRAPH_SVGALIB)
#define FILE16_TRANSP     MAKECOLOR16_real(0,0xff,0)
#define XBMP16_TRANSP     0
#define XBMP16_JACKCOLOR  MAKECOLOR16_real(0xff,0,0xff)
#else
#define XBMP16_TRANSP     MAKECOLOR16_real(0,0xff,0)
#define XBMP16_JACKCOLOR  MAKECOLOR16_real(0xff,0,0xff)
#endif

#if (defined GRAPH_DOS)
#define FILE32_TRANSP     MAKECOLOR32(0,0xff,0)
#define XBMP32_TRANSP     MASK_COLOR_32
#define XBMP32_JACKCOLOR  MAKECOLOR32(0,0xff,0)
#elif (defined GRAPH_SVGALIB)
#define FILE32_TRANSP     MAKECOLOR32(0,0xff,0)
#define XBMP32_TRANSP     0
#define XBMP32_JACKCOLOR  MAKECOLOR32(0xff,0,0xff)
#else
#define XBMP32_TRANSP     MAKECOLOR32(0,0xff,0)
#define XBMP32_JACKCOLOR  MAKECOLOR32(0xff,0,0xff)
#endif

#ifdef GRAPH_DOS
#include <allegro.h>
#endif /* GRAPH_DOS */

#ifdef GRAPH_WIN
extern struct SURFACE;
#endif /* GRAPH_WIN */

typedef struct XBITMAP {
  int w, h;                 /* Width, Height */
  int bpp;                  /* Bytes per pixel */
  int line_w;               /* Width of a line */
  int transparent;          /* 1 if must use `draw_bitmap_tr' to draw */
  int tr_first, tr_last;    /* First and last non-transparent lines */
#ifdef GRAPH_DOS
  BITMAP *allegro_bmp;         /* Allegro bitmap */
  COMPILED_SPRITE *compiled;   /* Compiled allegro bitmap */
#endif /* GRAPH_DOS */
#ifdef GRAPH_WIN
  struct SURFACE *bitmap;
#endif
  unsigned char *data;      /* Bitmap data */
  unsigned char *line[0];   /* The bitmap will actually have `h' lines */
} XBITMAP;

/* When we find a 16bpp bitmap in a file, read and convert to: */
extern int convert_16bpp_to;

/* Color corrections to to when reading a 16bpp file (delta for R,G,B): */
extern int color_correction[3];

/* This is to ease the 32bpp color manipulation: */
typedef struct COLOR_32BPP {
  unsigned char c[4];
} COLOR_32BPP;

XBITMAP *create_xbitmap(int w, int h, int bpp);
void destroy_xbitmap(XBITMAP *bmp);

void clear_xbitmap(XBITMAP *bmp, int color);
int read_xbitmaps(char *filename, int max, XBITMAP **bmp);
XBITMAP *read_xbitmap(char *filename);
int write_xbitmaps(char *filename, int n, XBITMAP **bmp);
int write_xbitmap(char *filename, XBITMAP *bmp);

typedef struct BMP_FONT {
  int n;
  XBITMAP *bmp[256];
} BMP_FONT;

BMP_FONT *read_bmp_font(char *filename);
int write_bmp_font(char *filename, BMP_FONT *font);
void destroy_bmp_font(BMP_FONT *font);


#endif /* BITMAP_H_FILE */
