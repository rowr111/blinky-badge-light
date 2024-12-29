#include "led_control.h"
#include "led_strip.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "LED_CONTROL";

// Static variables
static led_strip_handle_t strip;
static int current_pattern = 0; // Active pattern ID
static uint8_t brightness = MAX_BRIGHTNESS;
static const uint8_t brightness_levels[] = {MAX_BRIGHTNESS / 10, MAX_BRIGHTNESS / 5, MAX_BRIGHTNESS / 2, (3 * MAX_BRIGHTNESS) / 4, MAX_BRIGHTNESS, MAX_BRIGHTNESS}; // 10%, 20%, 50%, 75%, 100%, 100%
static int brightness_index = 0; // Index for the current brightness level

// Function to convert HSV to RGB
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float hf = h / 255.0f * 360.0f;
    float sf = s / 255.0f;
    float vf = v / 255.0f;

    int i = (int)floor(hf / 60.0f) % 6;
    float f = hf / 60.0f - i;
    float p = vf * (1.0f - sf);
    float q = vf * (1.0f - f * sf);
    float t = vf * (1.0f - (1.0f - f) * sf);

    switch (i) {
        case 0:
            *r = vf * 255;
            *g = t * 255;
            *b = p * 255;
            break;
        case 1:
            *r = q * 255;
            *g = vf * 255;
            *b = p * 255;
            break;
        case 2:
            *r = p * 255;
            *g = vf * 255;
            *b = t * 255;
            break;
        case 3:
            *r = p * 255;
            *g = q * 255;
            *b = vf * 255;
            break;
        case 4:
            *r = t * 255;
            *g = p * 255;
            *b = vf * 255;
            break;
        case 5:
            *r = vf * 255;
            *g = p * 255;
            *b = q * 255;
            break;
    }
}    

// Initialize LED strip
void init_leds() {
    ESP_LOGI(TAG, "Initializing LEDs");

    // Configure LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = LED_COUNT,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));

}

// Set the active pattern
void set_pattern(int pattern_id) {
    current_pattern = pattern_id % NUM_PATTERNS;
    update_leds();
}

// Set LED brightness
void set_brightness(int index) {
    ESP_LOGI(TAG, "Updating LEDs with brightness %d", index);
    brightness_index = index % (sizeof(brightness_levels) / sizeof(brightness_levels[0]));
    brightness = brightness_levels[brightness_index];
    update_leds();
}

// Update LED state based on the current pattern and brightness
void update_leds() {
    ESP_LOGI(TAG, "Updating LEDs with pattern %d", current_pattern);

    switch (current_pattern) {
        case 0: // Solid color (red)
            for (int i = 1; i < LED_COUNT; i++) {
                led_strip_set_pixel(strip, i, 255, 0, 0);
            }
            break;
        case 1: // Solid color (green)
            for (int i = 1; i < LED_COUNT; i++) {
                led_strip_set_pixel(strip, i, 0, 255, 0);
            }
            break;
        case 2: // Solid color (blue)
            for (int i = 1; i < LED_COUNT; i++) {
                led_strip_set_pixel(strip, i, 0, 0, 255);
            }
            break;
        case 3: // Rainbow
            for (int i = 1; i < LED_COUNT; i++) {
                uint8_t r = (i * 10) % 255;
                uint8_t g = (i * 20) % 255;
                uint8_t b = (i * 30) % 255;
                led_strip_set_pixel(strip, i, (r * brightness) / MAX_BRIGHTNESS,
                                                           (g * brightness) / MAX_BRIGHTNESS,
                                                           (b * brightness) / MAX_BRIGHTNESS);
            }
            break;
        case 4: // Off
            led_strip_clear(strip);
            return; // Nothing more to do
    }
    led_strip_refresh(strip); // Apply changes
}
