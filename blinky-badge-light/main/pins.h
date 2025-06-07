#ifndef PINS_H
#define PINS_H

#include <driver/gpio.h>

// LED pins
#define LED_PIN GPIO_NUM_2 // GPIO pin for LED data
#define LED_COUNT 24 // Number of LEDs

// Battery monitoring pin
#define ADC_CHANNEL ADC_CHANNEL_0  // GPIO36

#endif // PINS_H
