#include <driver/i2c.h>

#include "wii_i2c.h"

#define WII_I2C_PORT  I2C_NUM_0
#define WII_I2C_ADDR  0x52

static uint8_t data_init1[] = { 0xf0, 0x55 };
static uint8_t data_init2[] = { 0xfb, 0x00 };
static uint8_t data_req_ident[] = { 0xfa };
static uint8_t data_req[] = { 0x00 };
static uint8_t data[6];

static esp_err_t wii_i2c_setup_i2c(int sda_gpio, int scl_gpio)
{
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num    = sda_gpio,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num    = scl_gpio,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000,  // 100KHz
  };
  i2c_param_config(WII_I2C_PORT, &conf);
  return i2c_driver_install(WII_I2C_PORT, conf.mode, 0, 0, 0);
}

static esp_err_t wii_i2c_write(uint8_t *data, size_t len)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (WII_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write(cmd, data, len, true);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(WII_I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t wii_i2c_read(uint8_t *data, size_t len)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (WII_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(WII_I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

int wii_i2c_init(int sda_gpio, int scl_gpio)
{
  if (wii_i2c_setup_i2c(sda_gpio, scl_gpio) != ESP_OK) return 1;
  if (wii_i2c_write(data_init1, sizeof(data_init1)) != ESP_OK) return 1;
  if (wii_i2c_write(data_init2, sizeof(data_init2)) != ESP_OK) return 1;
  return 0;
}

const unsigned char *wii_i2c_read_ident(void)
{
  if (wii_i2c_write(data_req_ident, sizeof(data_req_ident)) != ESP_OK) return NULL;
  if (wii_i2c_read(data, sizeof(data)) != ESP_OK) return NULL;
  return (const unsigned char *) data;
}

int wii_i2c_request_state(void)
{
  if (wii_i2c_write(data_req, sizeof(data_req)) != ESP_OK) return 1;
  return 0;
}

const unsigned char *wii_i2c_read_state(void)
{
  if (wii_i2c_read(data, sizeof(data)) != ESP_OK) return NULL;
  return (const unsigned char *) data;
}

unsigned int wii_i2c_decode_ident(const unsigned char *ident)
{
  return ((ident[5] <<  0) |
          (ident[4] <<  8) |
          (ident[3] << 16) |
          (ident[2] << 24));
}

void wii_i2c_decode_nunchuck(const unsigned char *data, struct wii_i2c_nunchuck_state *state)
{
  state->x = data[0] - (1<<7);
  state->y = data[1] - (1<<7);
  state->acc_x = ((data[2] << 2) | ((data[5] & 0x0c) >> 2)) - (1<<9);
  state->acc_y = ((data[3] << 2) | ((data[5] & 0x30) >> 4)) - (1<<9);
  state->acc_z = ((data[4] << 2) | ((data[5] & 0xc0) >> 6)) - (1<<9);
  state->c = (data[5] & 0x02) ? 0 : 1;
  state->z = (data[5] & 0x01) ? 0 : 1;
}

void wii_i2c_decode_classic(const unsigned char *data, struct wii_i2c_classic_state *state)
{
  state->lx = (data[0] & 0x3f) - (1<<5);
  state->ly = (data[1] & 0x3f) - (1<<5);
  state->rx = (((data[0] & 0xc0) >> 3) | ((data[1] & 0xc0) >> 5) | (data[2] >> 7)) - (1<<4);
  state->ry = (data[2] & 0x1f) - (1<<4);
  
  state->a_lt = ((data[2] & 0x60) >> 2) | ((data[3] & 0xe0) >> 5);
  state->a_rt = data[3] & 0x1f;
  state->d_lt    = (data[4] & 0x20) ? 0 : 1;
  state->d_rt    = (data[4] & 0x02) ? 0 : 1;
  
  state->left  = (data[5] & 0x02) ? 0 : 1;
  state->right = (data[4] & 0x80) ? 0 : 1;
  state->up    = (data[5] & 0x01) ? 0 : 1;
  state->down  = (data[4] & 0x40) ? 0 : 1;
  state->a     = (data[5] & 0x10) ? 0 : 1;
  state->b     = (data[5] & 0x40) ? 0 : 1;
  state->x     = (data[5] & 0x08) ? 0 : 1;
  state->y     = (data[5] & 0x20) ? 0 : 1;
  state->plus  = (data[4] & 0x04) ? 0 : 1;
  state->minus = (data[4] & 0x10) ? 0 : 1;
  state->home  = (data[4] & 0x08) ? 0 : 1;
  state->zl    = (data[5] & 0x80) ? 0 : 1;
  state->zr    = (data[5] & 0x04) ? 0 : 1;
}
