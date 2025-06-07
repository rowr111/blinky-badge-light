#ifndef PINS_H
#define PINS_H

#include <driver/gpio.h>

// LED pins
#define LED_PIN GPIO_NUM_2 // GPIO pin for LED data
#define LED_COUNT 24 // Number of LEDs

// Battery monitoring pin
#define ADC_CHANNEL ADC_CHANNEL_0  // GPIO36

// Microphone pins
#define I2S_SCK_PIN GPIO_NUM_40  // Clock Pin (BCLK), pin 33
#define I2S_WS_PIN  GPIO_NUM_39  // Word Select (LRCLK), pin 32
#define I2S_DI_PIN  GPIO_NUM_38  // Data In (DOUT), pin 31

#endif // PINS_H
