#ifndef PINS_H
#define PINS_H

// LED pins
#define LED_PIN 16 // GPIO pin for LED data
#define LED_COUNT 10 // Number of LEDs

// Touch pad pins
#define TOUCH_PAD_1 0 //gpio 4
#define TOUCH_PAD_2 2 //gpio 2
#define TOUCH_PAD_3 7 //gpio 0

// Battery monitoring pin
#define ADC_CHANNEL ADC_CHANNEL_0  // GPIO36

// Microphone pins
#define I2S_SCK_PIN GPIO_NUM_25  // Clock Pin (BCLK)
#define I2S_WS_PIN  GPIO_NUM_26  // Word Select (LRCLK)
#define I2S_DI_PIN  GPIO_NUM_27  // Data In (DOUT)

#endif // PINS_H
