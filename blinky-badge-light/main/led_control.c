#include <math.h>
#include <stdbool.h>

#include "driver/rmt_tx.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_utils.h"

#include "battery_monitor.h"
#include "genes.h"
#include "led_control.h"
#include "microphone.h"
#include "pins.h"
#include "vu_meter.h"
#include "battery_level_pattern.h"
#include "storage.h"

static const char *TAG = "LED_CONTROL";

static led_strip_handle_t strip;
static int current_pattern = 0; // Active pattern ID
uint8_t brightness = MAX_BRIGHTNESS;
uint8_t effective_brightness = MAX_BRIGHTNESS;
static const uint8_t brightness_levels[] = { //gamma corrected brightness levels for better perceived change between levels
    20,    
    35,   
    69,   
    121, 
    200  
};


static int brightness_index = 0; // Index for the current brightness level
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

    // Limit brightness if battery is low but not critical
    if (limit_brightness) {
        effective_brightness = brightness_levels[0];
    } else {
        effective_brightness = brightness;
    }

    // Show battery meter patern if enabled and timer didn't run out, otherwise, turn it off
    if (show_battery_meter) {
        int elapsed = (esp_timer_get_time() / 1000) - battery_meter_start_time;
        if (elapsed >= BATTERY_TOTAL_MS) {
            show_battery_meter = false;
        }
        else {
            render_battery_level_pattern(framebuffer, elapsed);
            return;
        }
    }

    // VU meter pattern shortcut
    if (index == NUM_PATTERNS - 1) {
        render_vu_meter_pattern(framebuffer, g, loop);
        return;
    }

    // Main pattern loop
    int tau = map_16(g->cd_rate, 0, 255, 700, 8000); // ms
    int64_t curtime = esp_timer_get_time() / 1000; // ms
    float twopi = 2.0f * (float)M_PI;
    float anim = twopi * ((float)(curtime % tau) / tau);

    for (int i = 0; i < count; i++) {
        // ---- HUE calculation ----
        uint32_t hue_temp;
        uint8_t hue;

        if (!g->hue_dir) { // 0 for clockwise, 1 for counter-clockwise
            hue_temp = ((256U / count) * i + (loop * g->hue_rate));
        } else {
            hue_temp = ((256U / count) * i - (loop * g->hue_rate));
        }

        hue_temp &= 0x1FF;
        if (hue_temp <= 0xFF)
            hue = (uint8_t)hue_temp;
        else
            hue = (uint8_t)(511 - hue_temp);

        // Map to user color span
        hue = (uint8_t)map_16((int16_t)hue, 0, 255, (int16_t)g->hue_base, (int16_t)g->hue_bound);

        // ---- VALUE (brightness sinusoid) ----
        float t = (float)i / (float)(count - 1);
        float phase = twopi * g->cd_period * t;
        float spacetime = (g->cd_dir > 128) ? (phase + anim) : (phase - anim);
        float base_val = 127.0f * (1.0f + cosf(spacetime)); // 0..254
        uint8_t val = (uint8_t)base_val;

        // ---- NONLINEARITY/GAMMA ----
        if (g->nonlin > 127)
            val = (uint8_t)(((uint16_t)val * (uint16_t)val) >> 8);

        // ---- APPLY EFFECTIVE BRIGHTNESS ----
        // Sound-reactive pattern brightness + effective_brightness for basic sound reactive pattern
        if (index == NUM_PATTERNS - 2) {
            // Scale brightness by dB_brightness_level and effective_brightness
            val = (uint8_t)(dB_brightness_level * effective_brightness);
        } else {
            val = (uint8_t)((val * effective_brightness) / 255);
        }
        
        // ---- HSV to RGB ----
        uint8_t r, gr, b;
        hsv_to_rgb(hue, g->sat, val, &r, &gr, &b);

        // ---- Write to framebuffer ----
        set_pixel(framebuffer, i, r, gr, b);
    }
}


void flash_feedback_pattern() {
    flash_active = true; // Pause the lighting task
    uint8_t framebuffer[LED_COUNT * 3] = {0};

    // Flash all LEDs with white light for a brief moment
    for (int i = 0; i < LED_COUNT; i++) {
        set_pixel(framebuffer, i, 60, 60, 60); // White but lets not blind people
    }
    update_leds(framebuffer);
    vTaskDelay(125 / portTICK_PERIOD_MS); // 125ms flash

    // Turn off LEDs
    memset(framebuffer, 0, sizeof(framebuffer));
    update_leds(framebuffer);
    flash_active = false; // Resume the lighting task
}


void safety_pattern(uint8_t *framebuffer) {
    uint8_t dim_red = brightness_levels[0] * 0.2f;
    uint8_t full_red = brightness_levels[0];;

    int64_t ms = esp_timer_get_time() / 1000;
    int slowdown_factor = 150; // Adjust for speed
    int shifted = (ms / slowdown_factor);

    for (int i = 0; i < LED_COUNT; i++) {
        int pattern_index = (i + shifted) % 4;
        switch (pattern_index) {
            case 0: // Off
                set_pixel(framebuffer, i, 0, 0, 0);
                break;
            case 1: // Dim
            case 3: // Dim
                set_pixel(framebuffer, i, dim_red, 0, 0);
                break;
            case 2: // Bright
                set_pixel(framebuffer, i, full_red, 0, 0);
                break;
        }
    }
}

// Lighting task
void lighting_task(void *param) {
    int loop = 0;
    uint8_t framebuffer[LED_COUNT * 3];

    while (1) { 
        if (!flash_active) { // Only render patterns if no feedback is active
            // Render the current pattern 
            if (force_safety_pattern) {
                safety_pattern(framebuffer);
            } else {
                render_pattern(settings.pattern_id, framebuffer, LED_COUNT, loop);
            }
            // Update LEDs
            update_leds(framebuffer);

            // Increment loop counter for animation
            loop = (loop + 1) % 256;

            // Delay for smooth animation (adjust as needed)
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
}