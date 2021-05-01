#ifndef WII_I2C_H_FILE
#define WII_I2C_H_FILE

#define WII_I2C_IDENT_NUNCHUCK  0xa4200000
#define WII_I2C_IDENT_CLASSIC   0xa4200101

#ifdef __cplusplus
extern "C" {
#endif

int wii_i2c_init(int sda_gpio, int scl_gpio);
const unsigned char *wii_i2c_read_ident(void);
unsigned int wii_i2c_decode_ident(const unsigned char *ident);
int wii_i2c_request_state(void);
const unsigned char *wii_i2c_read_state(void);

struct wii_i2c_nunchuck_state {
  // accelerometer
  short int acc_x;
  short int acc_y;
  short int acc_z;

  // analog stick:
  signed char x;
  signed char y;

  // buttons:
  char c;
  char z;
};

void wii_i2c_decode_nunchuck(const unsigned char *data, struct wii_i2c_nunchuck_state *state);
  
struct wii_i2c_classic_state {
  // analog sticks:
  signed char lx;
  signed char ly;
  signed char rx;
  signed char ry;

  // triggers (a_ is the analog part, d_ is the click bit):
  unsigned char a_lt;
  unsigned char a_rt;
  char d_lt;
  char d_rt;

  // d-pad:
  char up;
  char down;
  char left;
  char right;

  // buttons:
  char a;
  char b;
  char x;
  char y;

  // bumpers:
  char zr;
  char zl;

  // face buttons:
  char home;
  char plus;
  char minus;
};

void wii_i2c_decode_classic(const unsigned char *data, struct wii_i2c_classic_state *state);

#ifdef __cplusplus
}
#endif

#endif /* WII_I2C_H_FILE */
