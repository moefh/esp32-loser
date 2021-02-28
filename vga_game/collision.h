
#ifndef COLLISION_H_FILE
#define COLLISION_H_FILE

enum {      /* Flags returned by calc_movement() */
  CM_X_CLIPPED = 0x01,
  CM_Y_CLIPPED = 0x02
};

int calc_movement(int x, int y, int w, int h, int dx, int dy, int *ret_dx, int *ret_dy);

#endif /* COLLISION_H_FILE */
