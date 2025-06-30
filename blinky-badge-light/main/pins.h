#ifndef PINS_H
#define PINS_H

#include "driver/touch_pad.h"
#include <driver/gpio.h>

// LED pins
#define LED_PIN GPIO_NUM_2 // GPIO pin for LED data
#define LED_COUNT 24 // Number of LEDs

// Battery monitoring pins
#define ADC_CHANNEL ADC_CHANNEL_8   // GPIO9 on ADC1
#define ADC_GPIO    GPIO_NUM_17
#define BATTERY_MONITOR_ENABLE_PIN GPIO_NUM_46 // GPIO46 controls battery monitor transistor

// Power pins
#define MOSFET_GATE_PIN GPIO_NUM_21
#define BUTTON_PIN 27 // Power button

// Microphone pins
#define I2S_SCK_PIN GPIO_NUM_40  // Clock Pin (BCLK), pin 33
#define I2S_WS_PIN  GPIO_NUM_39  // Word Select (LRCLK), pin 32
#define I2S_DI_PIN  GPIO_NUM_38  // Data In (DOUT), pin 31


#define NUM_TOUCH_PADS 6
static const touch_pad_t touch_pads[NUM_TOUCH_PADS] = {
    TOUCH_PAD_NUM5, // Next pattern touchpad
    TOUCH_PAD_NUM6, // Brightness level touchpad
    TOUCH_PAD_NUM3, // Replace pattern touchpad
    TOUCH_PAD_NUM4, // OFF touchpad
    TOUCH_PAD_NUM7, // Battery check touchpad
    TOUCH_PAD_NUM8,  // ? spot touchpad (future use)
};

#endif // PINS_H
