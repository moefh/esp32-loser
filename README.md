# esp32-loser

This is an old game, [Loser Corps](https://github.com/moefh/loser-corps),
ported to the [ESP32](https://www.espressif.com/en/products/socs/esp32) microcontroller
using bitluni's [ESP32Lib](https://github.com/bitluni/ESP32Lib) to output the game
screen to a VGA monitor. It supports the Wiimote controller using takeru's
[Wiimote](https://github.com/takeru/Wiimote) library, but can also use a simple
Arduino joystick shield.

Click the image to see the video on Youtube:

[![The game](images/loser1.jpg)](https://www.youtube.com/watch?v=BvJ3HLKo6p4)

The VGA output has 2 bits per color channel (which results in an image of 64 colors). I use
some very simple homemade DACs built with a few resistors, but there are people around selling
nicer boards -- just google "ESP32 VGA board".

The only thing of note about this port is the way the images are prepared and copied to the
framebuffer. In order to make things fast, the image data is pre baked with the vsync and hsync
signal bits addedd to each pixel, which means that they'd have to be generated again if a VGA
mode with different vsync/hsync polarity is used.

The image data is copied 4 bytes at a time to the framebuffer, because copying one byte at a
time is simply too slow. This is way more annoying than one would expect, because (due to the
way the I2S bus works in the ESP32) the order of the bytes don't match the order of the pixels
in the screen, so lots of shifting and re-arranging data is required, with 4 different versions
of the copying code, one for each way the source image is aligned to the screen. That has to be
done twice -- once for drawing all image pixels (for the background) and another for drawing
images with transparent pixels (for sprites and objects in the foreground), which is much slower.

Here's a photo of the ESP32 with my homemade VGA board with the DACs:

![ESP32](images/loser2.jpg)

The jumpers coming down out of it are connected to a standard Arduino joystick shield.
