#!/bin/bash

arduino-cli upload -v -b esp32:esp32:esp32 -p /dev/ttyUSB0 ./vga_game
