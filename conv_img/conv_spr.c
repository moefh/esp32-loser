/* conv_spr.c */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap.h"

struct INFO {
  const char *progname;
  char *in_filename;
  char out_filename[1024];
  char var_name[1024];
  unsigned int sync_bits;
  unsigned int num_frames;
};

static unsigned int conv_pixel(unsigned int sync_bits, unsigned int pixel)
{
  unsigned int r = (((pixel>>16) & 0xff) >> 6) & 3;
  unsigned int g = (((pixel>> 8) & 0xff) >> 6) & 3;
  unsigned int b = (((pixel>> 0) & 0xff) >> 6) & 3;
  return sync_bits | (b<<4) | (g<<2) | (r<<0);
}

static void free_sprs(XBITMAP **sprs, int num_sprs)
{
  for (int i = 0; i < num_sprs; i++) {
    destroy_xbitmap(sprs[i]);
  }
}

static int conv_file(struct INFO *info)
{
  convert_16bpp_to = 32;
  XBITMAP *sprs[1024];
  int num_sprs = read_xbitmaps(info->in_filename, sizeof(sprs)/sizeof(sprs[0]), sprs);
  if (num_sprs == 0) {
    printf("ERROR: can't read '%s'\n", info->in_filename);
    return 1;
  }
  if (info->num_frames == 0 || info->num_frames > num_sprs) {
    info->num_frames = num_sprs;
  }

  FILE *out = fopen(info->out_filename, "wb");
  if (out == NULL) {
    free_sprs(sprs, num_sprs);
    printf("ERROR: can't open '%s'\n", info->out_filename);
    return 1;
  }

  int width = sprs[0]->w;
  if (width % 4 != 0) {
    width += 4 - width % 4;
  }

  printf("%s -> %s (name=%s, sync_bits=0x%02x, num_frames=%d)\r\n", info->in_filename, info->out_filename, info->var_name, info->sync_bits, info->num_frames);

  fprintf(out, "/* File generated automatically from %s */\r\n\r\n", info->in_filename);
  fprintf(out, "const int img_%s_width   = %d;\r\n", info->var_name, sprs[0]->w);
  fprintf(out, "const int img_%s_height  = %d;\r\n", info->var_name, sprs[0]->h);
  fprintf(out, "const int img_%s_stride  = %d;\r\n", info->var_name, width/4);
  fprintf(out, "const int img_%s_num_spr = %d;\r\n\r\n", info->var_name, info->num_frames);

  fprintf(out, "const unsigned int img_%s_data[] = {", info->var_name);
  int num_out = 0;
  for (int spr_num = 0; spr_num < info->num_frames; spr_num++) {
    XBITMAP *spr = sprs[spr_num];
    for (int y = 0; y < spr->h; y++) {
      for (int lx = 0; lx < width/4; lx++) {  // process 4 pixels at a time
        unsigned int pixels[4];
        for (int i = 0; i < 4; i++) {
          int x = 4*lx + i;
          pixels[i] = (x >= spr->w) ? 0 : ((spr->line[y][4*x+0] <<  0) |
                                           (spr->line[y][4*x+1] <<  8) |
                                           (spr->line[y][4*x+2] << 16) |
                                           (spr->line[y][4*x+3] << 24));
        }
        unsigned int v = ((conv_pixel(info->sync_bits, pixels[0])<<16) |
                          (conv_pixel(info->sync_bits, pixels[1])<<24) |
                          (conv_pixel(info->sync_bits, pixels[2])<< 0) |
                          (conv_pixel(info->sync_bits, pixels[3])<< 8));
        if (num_out++ % 8 == 0) {
          fprintf(out, "\r\n  ");
        }
        fprintf(out, "0x%08xu,", v);
      }
    }
  }
  fprintf(out, "\r\n};\r\n");
  
  fclose(out);
  free_sprs(sprs, num_sprs);
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
  info->num_frames = 0;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[1], "-h") == 0) {
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
      } else if (strcmp(argv[i], "-num-frames") == 0) {
        if (i+1 >= argc) {
          printf("%s: argument missing for option '%s'\n", info->progname, argv[i]);
          return 1;
        }
        if (parse_num_frames(info, argv[++i]) != 0) {
          return 1;
        }
      } else {
        printf("%s: unknown option: '%s'\n", info->progname, argv[i]);
      }
    } else {
      if (info->in_filename != NULL) {
        printf("%s: only one input file supported\n", info->progname);
        return 1;
      }
      info->in_filename = argv[i];
    }
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
