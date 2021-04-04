
#include <stdio.h>

//#include "castle.c"
//#include "castle3.c"
#include "color_test.c"

#define CONV_COMPONENT(v) (((v) >> 6) & 3)

int main(void)
{
  printf("const int image_width = %d;\n", gimp_image.width);
  printf("const int image_height = %d;\n", gimp_image.height);
  printf("const unsigned char image_data[] = {");

  int out_pos = 0;
  const unsigned char *data = gimp_image.pixel_data;
  for (int i = 0; i < gimp_image.width * gimp_image.height; i++) {
    unsigned char pixel = ((CONV_COMPONENT(data[2]) << 4) |
                           (CONV_COMPONENT(data[1]) << 2) |
                           (CONV_COMPONENT(data[0]) << 0));
    data += 3;
    if (out_pos++ % 32 == 0) {
      printf("\n");
    }
    printf("%d,", pixel);
  }
  printf("\n};\n");
}
