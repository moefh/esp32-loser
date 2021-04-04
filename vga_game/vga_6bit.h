
#ifndef VGA_6BIT_H_FILE
#define VGA_6BIT_H_FILE

#include <cstdint>

class VgaMode;
extern const VgaMode vga_mode_320x240;
extern const VgaMode vga_mode_288x240;
extern const VgaMode vga_mode_240x240;

void vga_init(const int *vga_pins, const VgaMode &mode = vga_mode_320x240, bool double_buffered = true);
void vga_swap_buffers(bool wait_vsync = true);
void vga_clear_screen(uint8_t color);
uint8_t **vga_get_framebuffer();
uint8_t vga_get_sync_bits();
int vga_get_xres();
int vga_get_yres();

#endif /* VGA_6BIT_H_FILE */
