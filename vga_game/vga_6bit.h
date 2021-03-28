
#ifndef VGA_6BIT_H_FILE
#define VGA_6BIT_H_FILE

#include <cstdint>

void vga_init(const int *vga_pins);
void vga_swap_buffers(bool wait_vsync = true);
void vga_clear_screen(uint8_t color);
uint8_t **vga_get_framebuffer();
uint8_t vga_get_sync_bits();

#endif /* VGA_6BIT_H_FILE */
