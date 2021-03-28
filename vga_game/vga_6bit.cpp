/**
 * === Bare-bones 6-bit color VGA output for the ESP32 =================
 *
 * Most of this code was inspired by or stolen from bitluni's
 * excellent ESP32Lib. I made this because ESP32Lib has a lot of extra
 * stuff I don't need (like 16 bit output, drawing functions,
 * etc.). My only addition an interrupt handler to count frames, which
 * is used to wait for the right time to swap framebuffers.
 *
 * === Overview ========================================================
 *
 * This code outputs a VGA signal to 8 user-selected GPIO pins, with
 * H-sync, V-sync and 6 color bits to be used as desired (usually 2
 * red, 2 green and 2 blue with 3 2-bit DACs).
 *
 * It allocates 2 framebuffers in DMA-capable memory and sets I2S
 * (port 1) to output one of them (together with other buffers set up
 * as back/front porch and vsync/hsync) via DMA, triggering an
 * interrupt at the end. Each byte of the framebuffer is sent to an
 * output pin selected by the user, which should be connected to a VGA
 * monitor using appropriate resistors and DACs. The two framebuffers
 * can be swapped so while one of them is being sent out, the other is
 * being drawn.
 *
 * Each byte of the framebuffer always contains the vsync and hsync
 * signal at bits 7 and 6 (the high bits). The other 6 bits are set by
 * the user: usually (msb)RRGGBB(lsb), but other configurations are
 * possible (e.g. grayscale, RRGGGB, etc.). Just use appropriate DACs
 * to convert the output pins to VGA voltage levels.
 *
 * One annoying aspect of this library is the wonky order of the
 * pixels in the framebuffer: starting at each line, the address of
 * the byte corresponding to each pixel is this (the pattern repeats
 * every 4 pixels):
 *
 *   pixel x-coord:   0   1   2   3   4   5   6   7  ...
 *    mem. address:  [2] [3] [0] [1] [6] [7] [4] [5] ...
 *
 * One easy way to adjust for that is to xor the X coordinate with 2,
 * like this:
 *
 *   framebuffer[y][x^2] = vga_get_sync_bits() | color_bits;
 *
 * But note that writing one byte at a time on the ESP32 is very slow
 * (for optimal results, write at least 4 bytes at a time).
 *
 * === To use the library ==============================================
 *
 * 1) Call vga_init() to set things up and start the VGA output.
 * 
 * 2) In your loop, call vga_get_framebuffer() to get a pointer to the
 * lines of the current back framebuffer, where you'll write the
 * pixels.  Don't forget to write the sync bits, which you can get
 * with vga_get_sync_bits(). They depend only on the video mode so
 * it's safe to store them in a variable at the start of the program.
 *
 * 3) When you're done drawing a frame, call vga_swap_buffers() to
 * swap the front and back buffers. By default, this function waits
 * for the current frame to end. It you don't want this, call it
 * passing `false` (but if you do this, expect a wandering horizontal
 * line on the screeb).
 *
 * =====================================================================
 */

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <rom/lldesc.h>
#include <soc/rtc.h>
#include <soc/i2s_reg.h>
#include <soc/i2s_struct.h>
#include <soc/io_mux_reg.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <driver/periph_ctrl.h>

#include "vga_6bit.h"

// VGA mode constants (320x240)
static const int h_front     = 8;
static const int h_sync      = 48;
static const int h_back      = 24;
static const int h_pixels    = 320;
static const int v_front     = 11;
static const int v_sync      = 2;
static const int v_back      = 31;
static const int v_pixels    = 480;
static const int v_div       = 2;
static const int pixel_clock = 12587500;
static const int h_polarity  = 1;
static const int v_polarity  = 1;

static const uint8_t vsync_inv_bit = v_polarity<<7;
static const uint8_t hsync_inv_bit = h_polarity<<6;
static const uint8_t vsync_bit = (1-v_polarity)<<7;
static const uint8_t hsync_bit = (1-h_polarity)<<6;
static const int sync_bits = vsync_inv_bit | hsync_inv_bit;

static const int x_res = h_pixels;
static const int y_res = v_pixels / v_div;

/*
 * We use 2 DMA buffer descriptors per VGA line, their buffers are:
 *
 *                  |--hsync--|       |------------h_pixels------------|
 *
 *           +------+---------+-------+--------------------------------+
 *           |      |         |       |                                |
 *           |  dma_buf_hblank_vnorm  |   dma_buf_vblank_vnorm         |
 *           |      |         |       |                                |
 *     ---   +------+---------+-------+--------------------------------+
 *      |    |      |         |       |                                |
 *    vsync  |  dma_buf_hblank_vsync  |   dma_buf_vblank_vsync         |
 *      |    |      |         |       |                                |
 *     ---   +------+---------+-------+--------------------------------+
 *           |      |         |       |                                |
 *           |  dma_buf_hblank_vnorm  |   dma_buf_vblank_vnorm         |
 *           |      |         |       |                                |
 *     ---   +------+---------+-------+--------------------------------+
 *      |    |      |         |       |                                |
 *  v_pixels |  dma_buf_hblank_vnorm  |   framebuffer (pixels)         |
 *      |    |      |         |       |                                |
 *     ---   +------+---------+-------+--------------------------------+
 */

// DMA buffer descriptors
static int dma_buf_desc_count;
static lldesc_t *dma_buf_desc;

// DMA buffers for hblank/vblank
static uint8_t *dma_buf_hblank_vnorm;       // horizontal blank, outside vsync
static uint8_t *dma_buf_hblank_vsync;       // horizontal blank, inside vsync
static uint8_t *dma_buf_vblank_vnorm;       // blank pixel data, outside vsync
static uint8_t *dma_buf_vblank_vsync;       // blank pixel data, inside vsync

// DMA buffers for pixels (framebuffers)
static int active_framebuffer;              // index of current active (display) framebuffer
static int back_framebuffer;                // index of current back (drawing) framebuffer
static uint8_t **framebuffer[2];            // framebuffer (pixel data + sync bits)

static intr_handle_t i2s_isr_handle;        // I2S interrupt handler (triggered at end of frame)
static volatile int DRAM_ATTR vga_frame_count = 0;    // incremented by I2S interrupt handler


/*
 * ==========================================================================
 * ===== MISC FUNCTIONS =====================================================
 * ==========================================================================
 */

static void die(const char *msg)
{
  Serial.print("\n\n**** ERROR: ");
  Serial.println(msg);
  while (true) {
    delay(100);
  }
}

static void check_alloc(void *alloc, const char *msg)
{
  if (! alloc) {
    die(msg);
  }
}

/*
 * ==========================================================================
 * ===== I2S BUFFERS ========================================================
 * ==========================================================================
 */

/*
 * Allocate an array of DMA buffer descriptors
 */
static lldesc_t *alloc_dma_buf_desc_array(int num)
{
  lldesc_t *d = (lldesc_t *) heap_caps_malloc(sizeof(lldesc_t) * num, MALLOC_CAP_DMA);
  if (d) {
    for (int i = 0; i < num; i++) {
      d[i].length = 0;
      d[i].size = 0;
      d[i].owner = 1;
      d[i].sosf = 0;
      d[i].buf = (uint8_t *) 0;
      d[i].offset = 0;
      d[i].empty = 0;
      d[i].eof = 0;
      d[i].qe.stqe_next = (lldesc_t *) 0;
    }
  }
  return d;
}

/*
 * Set buffer and length of a DMA buffer descriptor
 */
static void set_dma_buf_desc_buffer(lldesc_t *buf_desc, uint8_t *buffer, int len)
{
  buf_desc->length = len;
  buf_desc->size = len;
  buf_desc->buf = buffer;
}

/*
 * Allocate DMA buffers and descriptors for VGA output via I2S.
 *
 * This function allocates all descriptors, but only the buffers
 * outside the framebuffer area (vblank/hblank). The framebuffer lines
 * must be allocated later and filled in with
 * set_vga_i2s_active_framebuffer().
 */
static void allocate_vga_i2s_buffers()
{
  // prepare DMA buffers for hblank/vblank areas
  int hblank_len = (h_front + h_sync + h_back + 3) & 0xfffffffc;
  dma_buf_hblank_vnorm = (uint8_t *) heap_caps_malloc(hblank_len, MALLOC_CAP_DMA);
  dma_buf_hblank_vsync = (uint8_t *) heap_caps_malloc(hblank_len, MALLOC_CAP_DMA);
  dma_buf_vblank_vnorm = (uint8_t *) heap_caps_malloc((h_pixels+3) & 0xfffffffc, MALLOC_CAP_DMA);
  dma_buf_vblank_vsync = (uint8_t *) heap_caps_malloc((h_pixels+3) & 0xfffffffc, MALLOC_CAP_DMA);

  check_alloc(dma_buf_hblank_vnorm, "not enough DMA memory for I2S buffers");
  check_alloc(dma_buf_hblank_vsync, "not enough DMA memory for I2S buffers");
  check_alloc(dma_buf_vblank_vnorm, "not enough DMA memory for I2S buffers");
  check_alloc(dma_buf_vblank_vsync, "not enough DMA memory for I2S buffers");

  for (int i = 0; i < hblank_len; i++) {
    if (i < h_front || i >= h_front+h_sync) {
      dma_buf_hblank_vnorm[i^2] = hsync_inv_bit | vsync_inv_bit;
      dma_buf_hblank_vsync[i^2] = hsync_inv_bit | vsync_bit;
    } else {
      dma_buf_hblank_vnorm[i^2] = hsync_bit | vsync_inv_bit;
      dma_buf_hblank_vsync[i^2] = hsync_bit | vsync_bit;
    }
  }
  for (int i = 0; i < h_pixels; i++) {
    dma_buf_vblank_vnorm[i^2] = hsync_inv_bit | vsync_inv_bit;
    dma_buf_vblank_vsync[i^2] = hsync_inv_bit | vsync_bit;
  }

  // allocate DMA buffer descriptors
  dma_buf_desc_count = 2 * (v_front + v_sync + v_back + v_pixels);
  dma_buf_desc = alloc_dma_buf_desc_array(dma_buf_desc_count);
  check_alloc(dma_buf_desc, "not enough memory for DMA buffer descriptors");
  for (int i = 0; i < dma_buf_desc_count; i++) {
    dma_buf_desc[i].qe.stqe_next = &dma_buf_desc[(i+1)%dma_buf_desc_count];
  }
  dma_buf_desc[dma_buf_desc_count-1].eof = true;  // trigger interrupt at the end of last buffer

  // stitch together the DMA buffers to compose a full VGA frame
  int d = 0;
  for (int i = 0; i < v_front; i++) {
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_hblank_vnorm, hblank_len);
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_vblank_vnorm, h_pixels);
  }
  for (int i = 0; i < v_sync; i++) {
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_hblank_vsync, hblank_len);
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_vblank_vsync, h_pixels);
  }
  for (int i = 0; i < v_back; i++) {
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_hblank_vnorm, hblank_len);
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_vblank_vnorm, h_pixels);
  }
  for (int i = 0; i < v_pixels; i++) {
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], dma_buf_hblank_vnorm, hblank_len);
    set_dma_buf_desc_buffer(&dma_buf_desc[d++], (uint8_t *) 0, h_pixels);  // set later with set_vga_i2s_active_framebuffer()
  }
}

/*
 * Set the framebuffer memory as the buffers for the DMA buffer
 * descriptors for I2S output.
 */
static void set_vga_i2s_active_framebuffer(unsigned char **fb)
{
  for (int i = 0; i < v_pixels; i++) {
    set_dma_buf_desc_buffer(&dma_buf_desc[2*(v_front+v_sync+v_back+i)+1], fb[i/v_div], h_pixels);
  }
}

/*
 * ==========================================================================
 * ===== FRAMEBUFFER ========================================================
 * ==========================================================================
 */

static unsigned char **alloc_framebuffer()
{
  unsigned char **fb = (unsigned char **) malloc(sizeof(unsigned char *) * v_pixels/v_div);
  check_alloc(fb, "not enough memory for framebuffer");
  for (int i = 0; i < v_pixels/v_div; i++) {
    fb[i] = (uint8_t *) heap_caps_malloc((h_pixels+3) & 0xfffffffc, MALLOC_CAP_DMA);
    check_alloc(fb[i], "not enough DMA memory for framebuffer");
  }
  return fb;
}

static void clear_framebuffer(unsigned char **fb, uint8_t color)
{
  unsigned int clear_byte = sync_bits | (color & 0x3f);
  unsigned int clear_data = ((clear_byte << 24) |
                             (clear_byte << 16) |
                             (clear_byte <<  8) |
                             (clear_byte <<  0));
  for (int y = 0; y < y_res; y++) {
    unsigned int *line = (unsigned int *) fb[y];
    for (int x = 0; x < x_res/4; x++) {
      line[x] = clear_data;
    }
  }
}

/*
 * ==========================================================================
 * === I2S OUTPUT ===========================================================
 * ==========================================================================
 */

static void reset_i2s_txrx()
{
   I2S1.conf.tx_reset = 1;
   I2S1.conf.tx_reset = 0;
   I2S1.conf.rx_reset = 1;
   I2S1.conf.rx_reset = 0;
}

static void reset_i2s_fifo()
{
   I2S1.conf.rx_fifo_reset = 1;
   I2S1.conf.rx_fifo_reset = 0;
   I2S1.conf.tx_fifo_reset = 1;
   I2S1.conf.tx_fifo_reset = 0;
}

static void reset_i2s_dma()
{
   I2S1.lc_conf.in_rst  = 1;
   I2S1.lc_conf.in_rst  = 0;
   I2S1.lc_conf.out_rst = 1;
   I2S1.lc_conf.out_rst = 0;  
}

/*
 * Interrupt handler function: just increment the frame count to let
 * the main logic to know when it's safe to swap framebuffers.
 */
static void IRAM_ATTR i2s_isr(void *arg)
{
  REG_WRITE(I2S_INT_CLR_REG(1), (REG_READ(I2S_INT_RAW_REG(1)) & 0xffffffc0) | 0x3f); // 1 means I2S1
  vga_frame_count++;
}

/*
 * Prepare I2S output to the selected pins (see comments on function vga_init())
 */
static void setup_i2s_output(const int *pin_map)
{
  periph_module_enable(PERIPH_I2S1_MODULE);
  for (int i = 0; i < 8; i++) {
    int pin = pin_map[i];
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
    gpio_set_direction((gpio_num_t) pin, (gpio_mode_t) GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(pin, I2S1O_DATA_OUT0_IDX + i, false, false);
  }
  periph_module_enable(PERIPH_I2S1_MODULE);

  // reset
  reset_i2s_txrx();
  reset_i2s_fifo();
  reset_i2s_dma();

  // set parallel mode
  I2S1.conf2.val = 0;
  I2S1.conf2.lcd_en = 1;
  I2S1.conf2.lcd_tx_wrx2_en = 1;
  I2S1.conf2.lcd_tx_sdx2_en = 0;
  
  // data rate
  I2S1.sample_rate_conf.val = 0;
  I2S1.sample_rate_conf.tx_bits_mod = 8;
  
  // clock setup
  long freq = pixel_clock * 2;
  int sdm, sdmn;
  int odir = -1;
  do {	
    odir++;
    sdm  = long((double(freq) / (20000000. / (odir + 2    ))) * 0x10000) - 0x40000;
    sdmn = long((double(freq) / (20000000. / (odir + 2 + 1))) * 0x10000) - 0x40000;
  } while(sdm < 0x8c0ecL && odir < 31 && sdmn < 0xA1fff);
  if (sdm > 0xA1fff) sdm = 0xA1fff;
  rtc_clk_apll_enable(true, sdm & 0xff, (sdm >> 8) & 0xff, sdm >> 16, odir);

  I2S1.clkm_conf.val = 0;
  I2S1.clkm_conf.clka_en = 1;
  I2S1.clkm_conf.clkm_div_num = 2;
  I2S1.clkm_conf.clkm_div_a = 1;
  I2S1.clkm_conf.clkm_div_b = 0;
  I2S1.sample_rate_conf.tx_bck_div_num = 1;

  I2S1.fifo_conf.val = 0;
  I2S1.fifo_conf.tx_fifo_mod_force_en = 1;
  I2S1.fifo_conf.tx_fifo_mod = 1;
  I2S1.fifo_conf.tx_data_num = 32;
  I2S1.fifo_conf.dscr_en = 1;

  I2S1.conf1.val = 0;
  I2S1.conf1.tx_stop_en = 0;
  I2S1.conf1.tx_pcm_bypass = 1;
  
  I2S1.conf_chan.val = 0;
  I2S1.conf_chan.tx_chan_mod = 1;
  
  I2S1.conf.tx_right_first = 1;

  I2S1.timing.val = 0;
  
  I2S1.conf.tx_msb_right = 0;
  I2S1.conf.tx_msb_shift = 0;
  I2S1.conf.tx_mono = 0;
  I2S1.conf.tx_short_sync = 0;

  i2s_isr_handle = 0;
  esp_intr_alloc(ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM, i2s_isr, NULL, &i2s_isr_handle);
}

/*
 * Start the I2S output
 */
static void start_i2s_output()
{
  esp_intr_disable(i2s_isr_handle);

  // reset
  I2S1.lc_conf.val |=   I2S_IN_RST_M | I2S_OUT_RST_M | I2S_AHBM_RST_M | I2S_AHBM_FIFO_RST_M;
  I2S1.lc_conf.val &= ~(I2S_IN_RST_M | I2S_OUT_RST_M | I2S_AHBM_RST_M | I2S_AHBM_FIFO_RST_M);
  I2S1.conf.val |=   I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M;
  I2S1.conf.val &= ~(I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M);
  while (I2S1.state.rx_fifo_reset_back) {
    // wait
  }

  // setup
  I2S1.lc_conf.val |= I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
  I2S1.out_link.addr = (uint32_t) &dma_buf_desc[0];
  I2S1.out_link.start = 1;
  I2S1.int_clr.val = I2S1.int_raw.val;
  I2S1.int_ena.val = 0;

  // enable interrupt
  I2S1.int_ena.out_eof = 1;
  esp_intr_enable(i2s_isr_handle);

  // start output
  I2S1.conf.tx_start = 1;
}

/*
 * ==========================================================================
 * === API ==================================================================
 * ==========================================================================
 */

/**
 * Initialize VGA output.
 *
 * `pin_map` must point to 8 ints selecting the pins, in this order:
 *   [0] red   bit 0 (lsb)
 *   [1] red   bit 1 (msb)
 *   [2] green bit 0 (lsb)
 *   [3] green bit 1 (msb)
 *   [4] blue  bit 0 (lsb)
 *   [5] blue  bit 1 (msb)
 *   [6] H-sync
 *   [7] V-sync
 */
void vga_init(const int *pin_map)
{
  framebuffer[0] = alloc_framebuffer();
  framebuffer[1] = alloc_framebuffer();
  active_framebuffer = 0;
  back_framebuffer = 1;
  clear_framebuffer(framebuffer[active_framebuffer], 0);
  clear_framebuffer(framebuffer[back_framebuffer], 0);

  allocate_vga_i2s_buffers();
  set_vga_i2s_active_framebuffer(framebuffer[active_framebuffer]);

  setup_i2s_output(pin_map);
  start_i2s_output();

#if 0
  Serial.print("sync_bits="); Serial.println(sync_bits);
  for (int i = 0; i < dma_buf_desc_count; i++) {
    Serial.printf("%4d: %4d 0x%08x { %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ... }\n",
                  i, dma_buf_desc[i].length, (int32_t) dma_buf_desc[i].buf,
                  dma_buf_desc[i].buf[ 0], dma_buf_desc[i].buf[ 1], dma_buf_desc[i].buf[ 2], dma_buf_desc[i].buf[ 3],
                  dma_buf_desc[i].buf[ 4], dma_buf_desc[i].buf[ 5], dma_buf_desc[i].buf[ 6], dma_buf_desc[i].buf[ 7],
                  dma_buf_desc[i].buf[ 8], dma_buf_desc[i].buf[ 9], dma_buf_desc[i].buf[10], dma_buf_desc[i].buf[11]);
  }
#endif
}

/**
 * Swap the back (drawing) and active (display) framebuffers.
 *
 * If `wait_vsync` is true, wait for a vertical sync before
 * swapping. Use false only for measure your drawing code performance,
 * since it will result in an annoying horizontal line on the screen.
 */
void vga_swap_buffers(bool wait_vsync)
{
  if (wait_vsync) {
    // we might wait up to 1/60s = 16.7 milliseconds here
    vga_frame_count = 0;
    while (vga_frame_count == 0) {
      delayMicroseconds(0);  // wait
    }
  }
  active_framebuffer = back_framebuffer;
  back_framebuffer = 1 - back_framebuffer;
  set_vga_i2s_active_framebuffer(framebuffer[active_framebuffer]);
}

/**
 * Clear the back (drawing) framebufer to the given color (0b00BBGGRR).
 */
void vga_clear_screen(uint8_t color)
{
  clear_framebuffer(vga_get_framebuffer(), color);
}

/**
 * Return the sync bits to use in every framebuffer pixel
 */
uint8_t vga_get_sync_bits()
{
  return sync_bits;
}

/**
 * Return a pointer to the back (drawing) framebuffer lines.
 */
uint8_t **vga_get_framebuffer()
{
  return framebuffer[back_framebuffer];
}