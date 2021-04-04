/* bitmap.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "bitmap.h"
#include "gz_open.h"


int color_correction[3], convert_16bpp_to = 16;


/* Create and return a bitmap of dimensions w, h */
XBITMAP *create_xbitmap(int w, int h, int bpp)
{
  XBITMAP *bmp;
  int i;

  if (bpp != 1 && bpp != 2 && bpp != 4)
    return NULL;                 /* Invalid format */

  bmp = (XBITMAP *) malloc(sizeof(XBITMAP) + sizeof(char *) * h);
  if (bmp == NULL)
    return NULL;

  bmp->w = w;
  bmp->h = h;
  bmp->bpp = bpp;
  bmp->line_w = w * bpp;
  bmp->transparent = 1;
  bmp->tr_first = 0;
  bmp->tr_last = bmp->h - 1;

#ifdef GRAPH_DOS
  bmp->allegro_bmp = create_bitmap_ex(bmp->bpp * 8, bmp->w, bmp->h);
  bmp->compiled = NULL;
  if (bmp->allegro_bmp == NULL) {
    free(bmp);
    return NULL;
  }
  bmp->data = bmp->allegro_bmp->dat;
  for (i = 0; i < h; i++)
    bmp->line[i] = bmp->allegro_bmp->line[i];
#else /* GRAPH_DOS */
  bmp->data = (unsigned char *) malloc(bmp->line_w * bmp->h * bmp->bpp);
  if (bmp->data == NULL) {
    free(bmp);
    return NULL;
  }
  for (i = 0; i < bmp->h; i++)
    bmp->line[i] = bmp->data + bmp->line_w * i;
#endif /* GRAPH_DOS */
  return bmp;
}

/* Free a bitmap */
void destroy_xbitmap(XBITMAP *bmp)
{
#ifdef GRAPH_DOS
  destroy_bitmap(bmp->allegro_bmp);
#else
  free(bmp->data);
#endif /* GRAPH_DOS */
  free(bmp);
}


/* Clear a bitmap to a color */
void clear_xbitmap(XBITMAP *bmp, int color)
{
  if (bmp->bpp == 1)
    memset(bmp->line[0], color, bmp->line_w * bmp->h);
  else if (bmp->bpp == 2) {
    int2bpp *s;
    int i;

    s = (int2bpp *) bmp->line[0];
    for (i = bmp->h * bmp->w; i > 0; i--)
      *s++ = color;
  } else {
    int4bpp *l;
    int i;

    l = (int4bpp *) bmp->line[0];
    for (i = bmp->h * bmp->w; i > 0; i--)
      *l++ = color;
  }
}

static short get_word(FILE *f)
{
  int c1, c2;

  c1 = fgetc(f) & 0xff;
  c2 = fgetc(f) & 0xff;
  return (c1 | (c2 << 8));
}

static long get_dword(FILE *f)
{
  int c1, c2, c3, c4;

  c1 = fgetc(f) & 0xff;
  c2 = fgetc(f) & 0xff;
  c3 = fgetc(f) & 0xff;
  c4 = fgetc(f) & 0xff;
  return (c1 | (c2 << 8) | (c3 << 16) | (c4 << 24));
}

static void put_dword(int s, FILE *f)
{
  fputc(s & 0xff, f);
  fputc((s >> 8) & 0xff, f);
  fputc((s >> 16) & 0xff, f);
  fputc((s >> 24) & 0xff, f);
}

static void put_word(short int s, FILE *f)
{
  fputc(s & 0xff, f);
  fputc((s >> 8) & 0xff, f);
}


static int read_bitmap_header(int *n, int *w, int *h, int *bpp, FILE *f)
{
  int magic;

  magic = get_dword(f);
  switch (magic) {
  case XBMP_MAGIC:
    *bpp = 1;
    break;

  case XBMP16_MAGIC:
    switch (convert_16bpp_to) {
    case 15: *bpp = 2; break;
    case 16: *bpp = 2; break;
    case 32: *bpp = 4; break;
    default: *bpp = 1; break;
    }
    if (*bpp < 2)
      return 1;
    break;

  default:
    printf("INVALID BITMAP MAGIC (%X != %X && %X != %X)\n",
	   magic, XBMP_MAGIC, magic, XBMP16_MAGIC);
    return 1;
  }
  *n = get_word(f);
  *w = get_word(f);
  *h = get_word(f);

  if (*n <= 0 || *w <= 0 || *h <= 0 || (*bpp != 1 && *bpp != 2 && *bpp != 4))
    return 1;
  return 0;
}


static void read_bitmap_data(XBITMAP *bmp, FILE *f)
{
  int x, y;

  bmp->transparent = 0;

  /* Read data from the file */
  if (bmp->bpp == 1) {
    for (x = 0; x < bmp->line_w; x++)
      for (y = 0; y < bmp->h; y++)
	bmp->line[y][x] = fgetc(f);
  } else if (bmp->bpp == 2) {
    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->line_w; x++)
	bmp->line[y][x] = fgetc(f);

    /* Convert to 15bpp if needed */
    if (convert_16bpp_to == 15) {
      int2bpp *p;

      for (y = 0; y < bmp->h; y++) {
	p = (int2bpp *) bmp->line[y];
	for (x = 0; x < bmp->w; x++, p++)
	  if (*p == XBMP16_TRANSP)
	    *p = XBMP15_TRANSP;
	  else {
	    int r, g, b;

	    r = *p >> 11;
	    g = ((*p >> 5) & 0x3f) >> 1;
	    b = *p & 0x1f;
	    if (r == 0 && b == 0 && g == 0x1f)
	      g = 0x1e;
	    *p = (r << 10) | (g << 5) | b;
	  }
      }
    }
  } else {  /* assume 4 bpp, convert from 2 bbp to 4 */
    unsigned short c, r, g, b;

    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->w; x++) {
	c = ((unsigned char)fgetc(f));
	c |= ((unsigned char) fgetc(f)) << 8;
	r = (c >> 11) << 3;
	g = ((c >> 5) & 0x3f) << 2;
	b = (c & 0x1f) << 3;
	if (MAKECOLOR16(r,g,b) == XBMP16_TRANSP)
	  ((int4bpp *)bmp->line[y])[x] = XBMP32_TRANSP;
	else
	  ((int4bpp *)bmp->line[y])[x] = MAKECOLOR32(r,g,b);
      }
  }

  /* Convert transparent pixels, if necessary */
#ifdef GRAPH_DOS
  if (bmp->bpp == 2) {
    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->w; x++)
	switch (((int2bpp *) bmp->line[y])[x]) {
	case FILE16_TRANSP:
	  ((int2bpp *) bmp->line[y])[x] = XBMP16_TRANSP;
	  break;
	case XBMP16_TRANSP:
	  ((int2bpp *) bmp->line[y])[x] = XBMP16_JACKCOLOR;
	  break;
	}
  }
#endif /* GRAPH_DOS */

#ifdef GRAPH_SVGALIB
  if (bmp->bpp == 2) {
    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->w; x++)
	switch (((int2bpp *) bmp->line[y])[x]) {
	case XBMP16_TRANSP:
	  ((int2bpp *) bmp->line[y])[x] = MAKECOLOR16(0,7,0);
	  break;
	case FILE16_TRANSP: 
	  ((int2bpp *) bmp->line[y])[x] = XBMP16_TRANSP;
	}
  } else if (bmp->bpp == 4) {
    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->w; x++)
	switch (((int4bpp *) bmp->line[y])[x]) {
	case XBMP32_TRANSP:
	  ((int4bpp *) bmp->line[y])[x] = MAKECOLOR32(0,7,0);
	  break;
	case FILE32_TRANSP: 
	  ((int4bpp *) bmp->line[y])[x] = XBMP32_TRANSP;
	}
  }
#endif /* GRAPH_SVGALIB */


  /* Make the color correction  */
  if (bmp->bpp == 2) {
    int2bpp *p;
    int r, g, b;

    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->w; x++) {
	p = ((int2bpp *) bmp->line[y]) + x;
	if (convert_16bpp_to == 15) {
	  if (*p != XBMP15_TRANSP) {
	    r = (*p >> 10) << 3;
	    g = ((*p >> 5) & 0x1f) << 3;
	    b = (*p & 0x1f) << 3;
	    r += color_correction[0];
	    g += color_correction[1];
	    b += color_correction[2];
	    if (r > 255)
	      r = 255;
	    if (g > 255)
	      g = 255;
	    if (b > 255)
	      b = 255;
	    *p = MAKECOLOR15_real(r, g, b);
	  }
	} else {
	  if (*p != XBMP16_TRANSP) {
	    r = (*p >> 11) << 3;
	    g = ((*p >> 5) & 0x3f) << 2;
	    b = (*p & 0x1f) << 3;
	    r += color_correction[0];
	    g += color_correction[1];
	    b += color_correction[2];
	    if (r > 255)
	      r = 255;
	    if (g > 255)
	      g = 255;
	    if (b > 255)
	      b = 255;
	    *p = MAKECOLOR16_real(r, g, b);
	  }
	}
      }
  } else if (bmp->bpp == 4) {
    int4bpp *pixel;

    for (y = 0; y < bmp->h; y++)
      for (x = 0; x < bmp->w; x++) {
	pixel = ((int4bpp *)bmp->line[y]) + x;
	if (*pixel != XBMP32_TRANSP) {
	  COLOR_32BPP *color = (COLOR_32BPP *) pixel;
	  short c;

	  c = color->c[0] + color_correction[0];
	  if (c > 255) c = 255;
	  color->c[0] = c;

	  c = color->c[1] + color_correction[1];
	  if (c > 255) c = 255;
	  color->c[1] = c;

	  c = color->c[2] + color_correction[2];
	  if (c > 255) c = 255;
	  color->c[2] = c;
	}
      }
  }

  /* Set transparent hint info */
  if (bmp->bpp == 1) {
    unsigned char *p = bmp->line[0];

    for (y = bmp->w * bmp->h - 1; y >= 0; y--, p++)
      if (*p == XBMP8_TRANSP) {
	bmp->transparent = 1;
	break;
      }
    
    if (bmp->transparent) {
      int found;

      /* Find the first and last non-transparent lines */
      found = 0;
      for (y = 0; y < bmp->h && ! found; y++)
	for (x = 0; x < bmp->w; x++) {
	  p = bmp->line[y] + x;
	  if (*p != XBMP8_TRANSP) {
	    bmp->tr_first = y;
	    found = 1;
	    break;
	  }
	}

      found = 0;
      for (y = bmp->h - 1; y >= 0 && ! found; y--)
	for (x = 0; x < bmp->w; x++) {
	  p = bmp->line[y] + x;
	  if (*p != XBMP8_TRANSP) {
	    bmp->tr_last = y;
	    found = 1;
	    break;
	  }
	}
    }
  } else if (bmp->bpp == 2) {
    int2bpp *p;

    if (convert_16bpp_to == 15) {
#if 0
      /* Find out if it is transparent */
      p = (int2bpp *) bmp->line[0];
      for (y = bmp->w * bmp->h - 1; y >= 0; y--, p++)
	if (*p == XBMP15_TRANSP) {
	  bmp->transparent = 1;
	  break;
	}

      if (bmp->transparent) {
	int found;
	
	/* Find the first and last non-transparent lines */
	found = 0;
	for (y = 0; y < bmp->h && ! found; y++)
	  for (x = 0; x < bmp->w; x++) {
	    p = ((int2bpp *) bmp->line[y]) + x;
	    if (*p != XBMP15_TRANSP) {
	      bmp->tr_first = y;
	      found = 1;
	      break;
	    }
	  }
	
	found = 0;
	for (y = bmp->h - 1; y >= 0 && ! found; y--)
	  for (x = 0; x < bmp->w; x++) {
	    p = ((int2bpp *) bmp->line[y]) + x;
	    if (*p != XBMP15_TRANSP) {
	      bmp->tr_last = y;
	      found = 1;
	      break;
	    }
	  }
      }
#else
      bmp->transparent = 1;
      bmp->tr_first = 0;
      bmp->tr_last = bmp->h - 1;
#endif
    } else {
      /* Find out if it is transparent */
      p = (int2bpp *) bmp->line[0];
      for (y = bmp->w * bmp->h - 1; y >= 0; y--, p++)
	if (*p == XBMP16_TRANSP) {
	  bmp->transparent = 1;
	  break;
	}

      if (bmp->transparent) {
	int found;
	
	/* Find the first and last non-transparent lines */
	found = 0;
	for (y = 0; y < bmp->h && ! found; y++)
	  for (x = 0; x < bmp->w; x++) {
	    p = ((int2bpp *) bmp->line[y]) + x;
	    if (*p != XBMP16_TRANSP) {
	      bmp->tr_first = y;
	      found = 1;
	      break;
	    }
	  }
	
	found = 0;
	for (y = bmp->h - 1; y >= 0 && ! found; y--)
	  for (x = 0; x < bmp->w; x++) {
	    p = ((int2bpp *) bmp->line[y]) + x;
	    if (*p != XBMP16_TRANSP) {
	      bmp->tr_last = y;
	      found = 1;
	      break;
	    }
	  }
      }
    }
  } else {  /* assume 4 bpp */
    int4bpp *p;

    /* Find out if it is transparent */
    for (y = bmp->h - 1; y >= 0 && ! bmp->transparent; y--)
      for (x = bmp->w - 1; x >= 0; x--) {
	p = ((int4bpp *)bmp->line[y]) + x;
	if (*p == XBMP32_TRANSP) {
	  bmp->transparent = 1;
	  break;
	}
      }

    if (bmp->transparent) {
      int found;

      /* Find the first and last non-transparent lines */
      found = 0;
      for (y = 0; y < bmp->h && ! found; y++)
	for (x = 0; x < bmp->w; x++) {
	  p = ((int4bpp *)bmp->line[y]) + x;
	  if (*p != XBMP32_TRANSP) {
	    bmp->tr_first = y;
	    found = 1;
	    break;
	  }
	}

      found = 0;
      for (y = bmp->h - 1; y >= 0 && ! found; y--)
	for (x = 0; x < bmp->w; x++) {
	  p = ((int4bpp *)bmp->line[y]) + x;
	  if (*p != XBMP32_TRANSP) {
	    bmp->tr_last = y;
	    found = 1;
	    break;
	  }
	}
    }
  }
}

/* Read an array of bitmaps from a file */
static int read_bitmap_array(XBITMAP **bmp, int n, int w, int h, int bpp,
			     FILE *f)
{
  int i;

  for (i = 0; i < n; i++) {
    if ((bmp[i] = create_xbitmap(w, h, bpp)) == NULL) {
      for (i--; i >= 0; i--)
	destroy_xbitmap(bmp[i]);
      return 1;
    }

    read_bitmap_data(bmp[i], f);
  }
  return 0;
}

/* Reads at most `max' bitmaps from `filename'. Returns the number
 * of bitmaps read. */
int read_xbitmaps(char *filename, int max, XBITMAP **bmp)
{
  GZIP_FILE gzfile;
  int n, w, h, bpp;

  gz_file_open(&gzfile, filename, "r");
  if (gzfile.f == NULL)
    return 0;
  if (read_bitmap_header(&n, &w, &h, &bpp, gzfile.f)) {
    gz_file_close(&gzfile);
    return 0;
  }
  if (n > max)
    n = max;
  if (read_bitmap_array(bmp, n, w, h, bpp, gzfile.f))
    n = 0;
  gz_file_close(&gzfile);
  return n;
}

/* Read a bitmap from a file */
XBITMAP *read_xbitmap(char *filename)
{
  XBITMAP *bmp;
  
  if (read_xbitmaps(filename, 1, &bmp) != 1)
    return NULL;
  return bmp;
}

/* Write and array of bitmaps to a file */
int write_xbitmaps(char *filename, int n, XBITMAP **bmp)
{
  GZIP_FILE gzfile;
  int i, x, y;

  gz_file_open(&gzfile, filename, "w");

  if (gzfile.f == NULL)
    return 1;

  if (bmp[0]->bpp == 1)
    put_dword(XBMP_MAGIC, gzfile.f);
  else
    put_dword(XBMP16_MAGIC, gzfile.f);
  put_word(n, gzfile.f);
  put_word(bmp[0]->w, gzfile.f);
  put_word(bmp[0]->h, gzfile.f);

  for (i = 0; i < n; i++)
    if (bmp[i]->bpp == 1)
      for (x = 0; x < bmp[i]->line_w; x++)
	for (y = 0; y < bmp[i]->h; y++)
	  fputc(bmp[i]->line[y][x], gzfile.f);
    else
      for (y = 0; y < bmp[i]->h; y++)
	for (x = 0; x < bmp[i]->line_w; x++)
	  fputc(bmp[i]->line[y][x], gzfile.f);

  gz_file_close(&gzfile);
  return 0;
}

/* Write a bitmap to a file */
int write_xbitmap(char *filename, XBITMAP *bmp)
{
  return write_xbitmaps(filename, 1, &bmp);
}


/* Read a font character bitmap from a bitmap font file */
static XBITMAP *read_font_char(FILE *f, int bpp)
{
  XBITMAP *bmp;
  int w, h;

  w = get_word(f);
  h = get_word(f);
  if (w <= 0 || h <= 0)
    return NULL;

  bmp = create_xbitmap(w, h, bpp);
  if (bmp == NULL)
    return NULL;

  read_bitmap_data(bmp, f);
  return bmp;
}

/* Read a font from a bitmap font file */
BMP_FONT *read_bmp_font(char *filename)
{
  GZIP_FILE gzfile;
  BMP_FONT *font;
  int i, bpp;

  gz_file_open(&gzfile, filename, "r");

  if (gzfile.f == NULL)
    return NULL;

  i = get_dword(gzfile.f);
  switch (i) {
  case FONT_MAGIC:
    bpp = 1;
    break;

  case FONT16_MAGIC:
    switch (convert_16bpp_to) {
    case 15: bpp = 2; break;
    case 16: bpp = 2; break;
    case 32: bpp = 4; break;
    default: bpp = 1; break;
    }
    if (bpp != 2 && bpp != 4) {
      gz_file_close(&gzfile);
      return NULL;
    }
    break;

  default:
    gz_file_close(&gzfile);
    return NULL;
  }
  i = get_word(gzfile.f);
  if (i <= 0) {
    gz_file_close(&gzfile);
    return NULL;
  }
  font = (BMP_FONT *) malloc(sizeof(BMP_FONT));
  if (font == NULL) {
    gz_file_close(&gzfile);
    return NULL;
  }
  font->n = i;

  for (i = 0; i < font->n; i++)
    if ((font->bmp[i] = read_font_char(gzfile.f, bpp)) == NULL) {
      gz_file_close(&gzfile);
      for (; i >= 0; i--)
	destroy_xbitmap(font->bmp[i]);
      free(font);
      return NULL;
    }

  gz_file_close(&gzfile);
  return font;
}

/* Destroy a font */
void destroy_bmp_font(BMP_FONT *font)
{
  int i;

  for (i = 0; i < font->n; i++)
    destroy_xbitmap(font->bmp[i]);
  free(font);
}

int write_bmp_font(char *filename, BMP_FONT *font)
{
  GZIP_FILE gzfile;
  int i, x, y;

  gz_file_open(&gzfile, filename, "w");

  if (gzfile.f == NULL)
    return 1;

  if (font->bmp[0]->bpp == 1)
    put_dword(FONT_MAGIC, gzfile.f);
  else
    put_dword(FONT16_MAGIC, gzfile.f);

  put_word(font->n, gzfile.f);

  for (i = 0; i < font->n; i++) {
    put_word(font->bmp[i]->w, gzfile.f);
    put_word(font->bmp[i]->h, gzfile.f);

    if (font->bmp[i]->bpp == 1)
      for (x = 0; x < font->bmp[i]->line_w; x++)
	for (y = 0; y < font->bmp[i]->h; y++)
	  fputc(font->bmp[i]->line[y][x], gzfile.f);
    else
      for (y = 0; y < font->bmp[i]->h; y++)
	for (x = 0; x < font->bmp[i]->line_w; x++)
	  fputc(font->bmp[i]->line[y][x], gzfile.f);
  }
  gz_file_close(&gzfile);
  return 0;
}

