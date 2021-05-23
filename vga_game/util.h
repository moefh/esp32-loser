#ifndef UTIL_H_FILE
#define UTIL_H_FILE

#if ARDUINO_ARCH_ESP32
// compiling under Arduino Core

#include <Arduino.h>

#else /* ARDUINO_ARCH_ESP32 */
// compiling under pure ESP-IDF

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp32-hal-gpio.h"

#define delay(n) vTaskDelay((n) / portTICK_PERIOD_MS)
unsigned long millis();

#endif /* ARDUINO_ARCH_ESP32 */

#endif /* UTIL_H_FILE */
