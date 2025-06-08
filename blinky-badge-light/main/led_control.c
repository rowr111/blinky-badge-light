#include <math.h>
#include <stdbool.h>

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_utils.h"

#include "battery_monitor.h"
#include "genes.h"
#include "led_control.h"
#include "microphone.h"
#include "pins.h"

static const char *TAG = "LED_CONTROL";

// Static variables
static led_strip_handle_t strip;
static int current_pattern = 0; // Active pattern ID
static uint8_t brightness = MAX_BRIGHTNESS;
static const uint8_t brightness_levels[] = {MAX_BRIGHTNESS / 10, MAX_BRIGHTNESS / 5, MAX_BRIGHTNESS / 2, (3 * MAX_BRIGHTNESS) / 4, MAX_BRIGHTNESS, MAX_BRIGHTNESS}; // 10%, 20%, 50%, 75%, 100%, 100%
static int brightness_index = 0; // Index for the current brightness level
static int limited_brightness_index = 1; // Index for the limited brightness level
volatile bool flash_active = false;

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
    ESP_LOGI(TAG, "Updating LEDs with pattern %d", pattern_id);
    current_pattern = pattern_id % NUM_PATTERNS;
}

// Set LED brightness
void set_brightness(int index) {
    ESP_LOGI(TAG, "Updating LEDs with brightness %d", index);
    brightness_index = index % (sizeof(brightness_levels) / sizeof(brightness_levels[0]));
    brightness = brightness_levels[brightness_index];
}

void update_leds(uint8_t *framebuffer) {
    if (!strip) {
        ESP_LOGE(TAG, "LED strip not initialized");
        return;
    }

    // Update each pixel in the LED strip
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t g = framebuffer[i * 3 + 0]; // Green
        uint8_t r = framebuffer[i * 3 + 1]; // Red
        uint8_t b = framebuffer[i * 3 + 2]; // Blue
        ESP_ERROR_CHECK(led_strip_set_pixel(strip, i, r, g, b));
    }

    // Refresh the strip to apply the changes
    ESP_ERROR_CHECK(led_strip_refresh(strip));
}

void render_pattern(int index, uint8_t *framebuffer, int count, int loop) {
    genome *g = &patterns[index];
    uint8_t hue;
    Color color;

    // Determine the effective brightness
    uint8_t effective_brightness = brightness;
    if (limit_brightness) {
        uint8_t limited_brightness = brightness_levels[limited_brightness_index];
        if (limited_brightness < brightness) {
            effective_brightness = limited_brightness;
        }
    }

    for (int i = 0; i < count; i++) {
        hue = (g->hue_base + i * g->hue_ratedir + loop) % 256;
        float base_brightness = 127.0f + (127.0f * sinf(2 * M_PI * loop / 256));
        float eff_brightness = (base_brightness * effective_brightness) / 255.0f;

        // If sound reactive pattern, replace with dB_brightness_level
        if (current_pattern == NUM_PATTERNS - 1) {
            eff_brightness = dB_brightness_level * 255.0f;
        }

        // Clamp before converting to int (optional, avoids overflow)
        if (eff_brightness > 255.0f) eff_brightness = 255.0f;
        if (eff_brightness < 0.0f) eff_brightness = 0.0f;

        color = Wheel(hue);

        // Use float math to avoid rounding errors
        float sat = g->sat / 255.0f;
        color.r = (uint8_t)((color.r * eff_brightness * sat) / 255.0f);
        color.g = (uint8_t)((color.g * eff_brightness * sat) / 255.0f);
        color.b = (uint8_t)((color.b * eff_brightness * sat) / 255.0f);

        set_pixel(framebuffer, i, color.r, color.g, color.b);
    }
}

void flash_feedback_pattern() {
    flash_active = true; // Pause the lighting task
    uint8_t framebuffer[LED_COUNT * 3] = {0};

    // Flash all LEDs with white light for a brief moment
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 80, 80, 80); // White but lets not blind people
    }
    update_leds(framebuffer);
    vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms flash

    // Turn off LEDs
    memset(framebuffer, 0, sizeof(framebuffer));
    update_leds(framebuffer);
    flash_active = false; // Resume the lighting task
}

void safety_pattern(uint8_t *framebuffer, int count, int loop) {
    // Set brightness to 10% of MAX_BRIGHTNESS
    uint8_t safety_brightness = MAX_BRIGHTNESS / 10;

    // Define color intensities
    uint8_t lower_red = safety_brightness / 6; // Lower brightness red
    uint8_t full_red = safety_brightness;     // Full brightness red

    // Slow down the pattern cycle
    int slowdown_factor = 7;  // Adjust this to change the speed (higher = slower)
    int slowed_loop = loop / slowdown_factor;

    // Create the safety pattern
    for (int i = 0; i < count; i++) {
        int pattern_index = (i + slowed_loop) % 4; // Cycle through 4-step pattern

        switch (pattern_index) {
            case 0: // Black
                set_pixel(framebuffer, i, 0, 0, 0);
                break;
            case 1: // Lower brightness red
                set_pixel(framebuffer, i, lower_red, 0, 0);
                break;
            case 2: // Full brightness red
                set_pixel(framebuffer, i, full_red, 0, 0);
                break;
            case 3: // Lower brightness red
                set_pixel(framebuffer, i, lower_red, 0, 0);
                break;
        }
    }
}