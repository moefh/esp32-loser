/* gz_open.c
 *
 * Functions to deal with gzipped files
 */

#include <stdio.h>
#include <unistd.h>

#include "gz_open.h"

/* Open a file, possibly gunzipping it */
int gz_file_open(GZIP_FILE *file, const char *name, const char *mode)
{
  char file_name[1024];

  if ((file->f = fopen(name, mode)) != NULL) {
    file->piped = 0;
    return 0;
  }

  sprintf(file_name, "%s.gz", name);
  if (access(file_name, R_OK) == 0) {
    char command[1024 + 16];

    sprintf(command, "gunzip -c %s", file_name);
    if ((file->f = popen(command, mode)) != NULL) {
      file->piped = 1;
      return 0;
    }
  }

  return 1;
}

/* Close a file opened with file_open() */
int gz_file_close(GZIP_FILE *file)
{
  if (file->piped)
    return pclose(file->f);
  return fclose(file->f);
}
