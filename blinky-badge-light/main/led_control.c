#include "led_control.h"
#include "driver/rmt_tx.h"
#include "led_strip.h"
#include "led_utils.h"
#include "genes.h"
#include "battery_monitor.h"
#include "esp_log.h"
#include <math.h>
#include <stdbool.h>

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
        // Calculate hue and base brightness
        hue = (g->hue_base + i * g->hue_ratedir + loop) % 256;
        uint8_t base_brightness = 127 + (127 * sinf(2 * M_PI * loop / 256));

        // Scale brightness with effective_brightness
        uint8_t final_brightness = (base_brightness * effective_brightness) / 255;

        // Generate color using Wheel function
        color = Wheel(hue);

        // Adjust RGB values based on final brightness and saturation
        color.r = (color.r * final_brightness * g->sat) / (255 * 255);
        color.g = (color.g * final_brightness * g->sat) / (255 * 255);
        color.b = (color.b * final_brightness * g->sat) / (255 * 255);

        // Set the pixel in the framebuffer
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