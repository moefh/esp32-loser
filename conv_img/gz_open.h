/* gz_open.h */

#ifndef GZ_OPEN_H
#define GZ_OPEN_H

typedef struct GZIP_FILE {
  FILE *f;           /* The file stream */
  int piped;         /* 1 if opened with popen */
} GZIP_FILE;

int gz_file_open(GZIP_FILE *gzfile, const char *filename, const char *mode);
int gz_file_close(GZIP_FILE *gzfile);

#endif
