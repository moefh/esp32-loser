/* conv_bmp.c */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bmp.h"

struct INFO {
  const char *progname;
  char *in_filename;
  char out_filename[1024];
  char var_name[1024];
  unsigned int sync_bits;
  unsigned int num_frames;
  unsigned int num_tiles_x;
  unsigned int num_tiles_y;
  int dont_scramble_for_esp32;
  int dont_output_crlf;
};

struct IMAGE_READER {
  struct BMP_IMAGE *bmp;
  unsigned int w;
  unsigned int h;
  unsigned int num_tiles_x;
  unsigned int num_tiles_y;
};

static unsigned int conv_pixel(unsigned int sync_bits, unsigned int pixel)
{
  unsigned int r = (((pixel>>16) & 0xff) >> 6) & 3;
  unsigned int g = (((pixel>> 8) & 0xff) >> 6) & 3;
  unsigned int b = (((pixel>> 0) & 0xff) >> 6) & 3;
  return sync_bits | (b<<4) | (g<<2) | (r<<0);
}

static unsigned char *reader_get_image_line(struct IMAGE_READER *reader, int tile_x, int tile_y, int y)
{
  int start_x = tile_x * reader->w;
  int start_y = tile_y * reader->h + y;
  return &reader->bmp->data[start_y * reader->bmp->stride + start_x*(reader->bmp->bpp/8)];
}

static int conv_file(struct INFO *info)
{
  struct IMAGE_READER reader;
  reader.bmp = bmp_read(info->in_filename, BMP_FORMAT_BGRA);
  if (! reader.bmp) {
    printf("ERROR: can't read '%s'\n", info->in_filename);
    return 1;
  }
  reader.w = reader.bmp->w / info->num_tiles_x;
  reader.h = reader.bmp->h / info->num_tiles_y;
  reader.num_tiles_x = info->num_tiles_x;
  reader.num_tiles_y = info->num_tiles_y;
  if (info->num_frames == 0 || info->num_frames > info->num_tiles_x*info->num_tiles_y) {
    info->num_frames = info->num_tiles_x*info->num_tiles_y;
  }
   
  FILE *out = fopen(info->out_filename, "wb");
  if (out == NULL) {
    bmp_free(reader.bmp);
    printf("ERROR: can't open '%s'\n", info->out_filename);
    return 1;
  }

  int width = reader.h;
  if (width % 4 != 0) {
    width += 4 - width % 4;
  }

  const char *line_end = (info->dont_output_crlf) ? "\n" : "\r\n";
  
  printf("%s -> %s (name=%s, sync_bits=0x%02x, num_frames=%d)%s\n", info->in_filename, info->out_filename, info->var_name, info->sync_bits, info->num_frames, line_end);

  fprintf(out, "/* File generated automatically from %s */%s%s\n", info->in_filename, line_end, line_end);
  fprintf(out, "#define img_%s_width   %d%s\n", info->var_name, reader.w, line_end);
  fprintf(out, "#define img_%s_height  %d%s\n", info->var_name, reader.h, line_end);
  fprintf(out, "#define img_%s_stride  %d%s\n", info->var_name, width/4, line_end);
  fprintf(out, "#define img_%s_num_spr %d%s%s\n", info->var_name, info->num_frames, line_end, line_end);

  fprintf(out, "const unsigned int img_%s_data[] = {", info->var_name);
  int num_out = 0;
  int num_frames = 0;
  int bpp = reader.bmp->bpp / 8;
  for (int tile_x = 0; tile_x < reader.num_tiles_x; tile_x++) {
    for (int tile_y = 0; tile_y < reader.num_tiles_y; tile_y++) {
      if (num_frames++ > info->num_frames) {
        printf("stopping at frame %d\n", num_frames);
        break;
      }
        
      for (int y = 0; y < reader.h; y++) {
        unsigned char *line = reader_get_image_line(&reader, tile_x, tile_y, y);
        for (int lx = 0; lx < width/4; lx++) {  // process 4 pixels at a time
          unsigned int pixels[4];
          for (int i = 0; i < 4; i++) {
            int x = 4*lx + i;
            pixels[i] = (x >= reader.w) ? 0 : (((unsigned int)line[bpp*x+0] <<  0) |
                                               ((unsigned int)line[bpp*x+1] <<  8) |
                                               ((unsigned int)line[bpp*x+2] << 16) |
                                               ((unsigned int)line[bpp*x+3] << 24));
          }
          unsigned int v;
          if (info->dont_scramble_for_esp32) {
            v = ((conv_pixel(info->sync_bits, pixels[0])<< 0) |
                 (conv_pixel(info->sync_bits, pixels[1])<< 8) |
                 (conv_pixel(info->sync_bits, pixels[2])<<16) |
                 (conv_pixel(info->sync_bits, pixels[3])<<24));
          } else {
            v = ((conv_pixel(info->sync_bits, pixels[0])<<16) |
                 (conv_pixel(info->sync_bits, pixels[1])<<24) |
                 (conv_pixel(info->sync_bits, pixels[2])<< 0) |
                 (conv_pixel(info->sync_bits, pixels[3])<< 8));
          }
          if (num_out++ % 8 == 0) {
            fprintf(out, "%s  ", line_end);
          }
          fprintf(out, "0x%08xu,", v);
        }
      }
    }
  }
  fprintf(out, "%s};%s\n", line_end, line_end);
  
  fclose(out);
  bmp_free(reader.bmp);
  return 0;
}

static void print_help(const char *progname)
{
  printf("USAGE: %s [options] INPUT.spr\n", progname);
  printf("\n");
  printf("options:\n");
  printf("   -h              show this help\n");
  printf("   -out FILE       set output file (default: based on input file)\n");
  printf("   -name NAME      set C variable name (default: based on input file)\n");
  printf("   -sync BITS      set sync bits (default: 0x3c)\n");
  printf("   -frames N       set maximum num frames to convert (defaut: 0=unlimited)\n");
  printf("   -noscramble     don't scramble image as required by ESP32 I2S\n");
}

static int parse_sync_bits(struct INFO *info, const char *str)
{
  char *end = NULL;
  unsigned long sync_bits = strtoul(str, &end, 0);
  if (end == NULL || end == str || *end != '\0') {
    printf("ERROR: invalid sync_bits: must be a number\n");
    return 1;
  }
  info->sync_bits = (unsigned int) (sync_bits & 0xff);
  return 0;
}

static int parse_num_frames(struct INFO *info, const char *str)
{
  char *end = NULL;
  unsigned long num_frames = strtoul(str, &end, 0);
  if (end == NULL || end == str || *end != '\0') {
    printf("ERROR: invalid num_frames: must be a number\n");
    return 1;
  }
  info->num_frames = (unsigned int) num_frames;
  return 0;
}

static int parse_num_tiles(struct INFO *info, int coord, const char *str)
{
  char *end = NULL;
  unsigned long num_tiles = strtoul(str, &end, 0);
  if (end == NULL || end == str || *end != '\0') {
    printf("ERROR: invalid num tiles: must be a number\n");
    return 1;
  }
  if (coord == 0) {
    info->num_tiles_x = (unsigned int) num_tiles;
  } else {
    info->num_tiles_y = (unsigned int) num_tiles;
  }
  return 0;
}

static int make_default_out_filename(struct INFO *info, const char *prefix, const char *ext)
{
  size_t out_pos = 0;

  // copy directory
  const char *in_filename = info->in_filename;
  const char *last_slash = strrchr(in_filename, '/');
  if (last_slash != NULL) {
    size_t copy_len = last_slash - in_filename + 1;   // from start to last slash (including slash)
    if (out_pos + copy_len > sizeof(info->out_filename)) {
      printf("%s: input filename too long\n", info->progname);
      return 1;
    }
    memcpy(&info->out_filename[out_pos], in_filename, copy_len);
    out_pos += copy_len;
    in_filename += copy_len;
  }

  // add prefix
  size_t prefix_len = strlen(prefix);
  if (out_pos + prefix_len > sizeof(info->out_filename)) {
    printf("%s: input filename too long\n", info->progname);
    return 1;
  }
  memcpy(&info->out_filename[out_pos], prefix, prefix_len);
  out_pos += prefix_len;
  
  // copy file name without extension
  const char *last_dot = strrchr(in_filename, '.');
  size_t copy_len = (last_dot == NULL) ? strlen(in_filename) : last_dot - in_filename;
  if (out_pos + copy_len > sizeof(info->out_filename)) {
    return 1;
  }
  memcpy(&info->out_filename[out_pos], in_filename, copy_len);
  out_pos += copy_len;

  // copy new extension and trailing '\0'
  size_t ext_len = strlen(ext) + 1;
  if (out_pos + ext_len > sizeof(info->out_filename)) {
    return 1;
  }
  memcpy(&info->out_filename[out_pos], ext, ext_len);
  return 0;
}

static int make_default_var_name(struct INFO *info)
{
  const char *last_slash = strrchr(info->in_filename, '/');
  if (last_slash == NULL) {
    last_slash = info->in_filename;
  } else {
    last_slash++;  // skip slash
  }

  const char *last_dot = strrchr(last_slash, '.');
  if (last_dot == NULL) {
    last_dot = last_slash + strlen(last_slash);
  }

  size_t len = last_dot - last_slash;
  if (len + 1 > sizeof(info->var_name)) {
    return 1;
  }
  const char *in = last_slash;
  char *out = info->var_name;
  while (in != last_dot) {
    char c = *in++;
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && (c != '_')) {
      c = '_';
    }
    *out++ = c;
  }
  *out++ = '\0';
  return 0;
}

static int parse_args(struct INFO *info, int argc, char *argv[])
{
  info->in_filename = NULL;
  info->out_filename[0] = '\0';
  info->var_name[0] = '\0';
  info->sync_bits = 0;
  info->num_tiles_x = 1;
  info->num_tiles_y = 1;
  info->num_frames = 0;
  info->dont_scramble_for_esp32 = 0;
  info->dont_output_crlf = 0;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-h") == 0) {
        print_help(argv[0]);
        exit(0);
      } else if (strcmp(argv[i], "-out") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        snprintf(info->out_filename, sizeof(info->out_filename), "%s", argv[++i]);
      } else if (strcmp(argv[i], "-name") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        snprintf(info->var_name, sizeof(info->var_name), "%s", argv[++i]);
      } else if (strcmp(argv[i], "-sync") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        if (parse_sync_bits(info, argv[++i]) != 0) {
          return 1;
        }
      } else if (strcmp(argv[i], "-num-tiles-x") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        if (parse_num_tiles(info, 0, argv[++i]) != 0) {
          return 1;
        }
      } else if (strcmp(argv[i], "-num-tiles-y") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        if (parse_num_tiles(info, 1, argv[++i]) != 0) {
          return 1;
        }
      } else if (strcmp(argv[i], "-num-frames") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        if (parse_num_frames(info, argv[++i]) != 0) {
          return 1;
        }
      } else if (strcmp(argv[i], "-no-scramble") == 0) {
        info->dont_scramble_for_esp32 = 1;
      } else if (strcmp(argv[i], "-no-crlf") == 0) {
        info->dont_output_crlf = 1;
      } else {
        printf("%s: unknown option: '%s'\n", info->progname, argv[i]);
        return 1;
      }
    } else {
      if (info->in_filename != NULL) {
        printf("%s: only one input file supported\n", info->progname);
        return 1;
      }
      info->in_filename = argv[i];
    }
  }

  if (! info->in_filename) {
    printf("%s: no input file!\n", argv[0]);
    return 1;
  }
  
  if (info->out_filename[0] == '\0') {
    if (make_default_out_filename(info, "spr_", ".h") != 0) {
      printf("%s: input filename too long\n", info->progname);
      return 1;
    }
  }
  if (info->var_name[0] == '\0') {
    if (make_default_var_name(info) != 0) {
      printf("%s: input filename too long\n", info->progname);
      return 1;
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  static struct INFO info;
  info.progname = argv[0];
  if (parse_args(&info, argc, argv) != 0) {
    return 1;
  }

  return conv_file(&info);
}
