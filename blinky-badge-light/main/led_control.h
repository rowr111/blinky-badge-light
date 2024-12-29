#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

// Function prototypes
void init_leds();
void set_pattern(int pattern_id);
void set_brightness();
void update_leds();

// Constants
#define LED_PIN 16              // GPIO pin for LED data
#define LED_COUNT 10            // Number of LEDs
#define MAX_BRIGHTNESS 255      // Maximum brightness level
#define NUM_PATTERNS 5          // Number of lighting patterns
#define NUM_BRIGHTNESS_LEVELS 5 // Number of brightness levels

#endif // LED_CONTROL_H