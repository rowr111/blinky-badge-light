#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

// Function to initialize LEDs
void init_leds(void);
void set_pattern(int pattern_id);
void set_brightness();
// Function to update LEDs with the framebuffer
void update_leds(uint8_t *framebuffer);
// Function to render a pattern based on the index and loop
void render_pattern(int index, uint8_t *framebuffer, int count, int loop);


// Constants
#define LED_PIN 16              // GPIO pin for LED data
#define LED_COUNT 10            // Number of LEDs
#define MAX_BRIGHTNESS 255      // Maximum brightness level
#define NUM_PATTERNS 5          // Number of lighting patterns
#define NUM_BRIGHTNESS_LEVELS 5 // Number of brightness levels

#endif // LED_CONTROL_H