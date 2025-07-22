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
int64_t flash_end_time = 0;

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
    esp_err_t err;
    // Update each pixel in the LED strip
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t g = framebuffer[i * 3 + 0]; // Green
        uint8_t r = framebuffer[i * 3 + 1]; // Red
        uint8_t b = framebuffer[i * 3 + 2]; // Blue
        err = led_strip_set_pixel(strip, i, r, g, b);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set pixel %d: %s", i, esp_err_to_name(err));
        }
    }

    // Refresh the strip to apply the changes
    err = led_strip_refresh(strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh LED strip: %s", esp_err_to_name(err));
    }
}


uint8_t hue_table[24] = {
      0,   // 0
     21,   // 1
     42,   // 2
     63,   // 3
     85,   // 4
    106,   // 5
    127,   // 6
    148,   // 7
    170,   // 8
    191,   // 9
    212,   // 10
    233,   // 11
    255,   // 12  
    233,   // 13
    212,   // 14
    191,   // 15
    170,   // 16
    148,   // 17
    127,   // 18
    106,   // 19
     85,   // 20
     63,   // 21
     42,   // 22
     21    // 23
};


uint8_t calculate_pattern_hue(const genome *g, int led_index, int loop) {
    int dir = (g->hue_dir == 0) ? 1 : -1;
    float frac_offset = ((float)(dir * loop * g->hue_rate) / 256.0f) * LED_COUNT;
    while (frac_offset < 0) frac_offset += LED_COUNT;
    while (frac_offset >= LED_COUNT) frac_offset -= LED_COUNT;

    uint8_t hue;
    if ((g->hue_base == 0) && (g->hue_bound == 255)) {
        // Full rainbow
        uint32_t base_hue = (255 * led_index) / LED_COUNT;
        int32_t animated_hue = base_hue + dir * (loop * g->hue_rate);
        hue = (uint8_t)(((animated_hue % 256) + 256) % 256);
    } else {
        // Limited hue range, triangle wave
        float shifted = (led_index + frac_offset);
        int idx0 = ((int)shifted) % LED_COUNT;
        int idx1 = (idx0 + 1) % LED_COUNT;
        float frac = shifted - (int)shifted;
        uint8_t base_hue = (uint8_t)((1.0f - frac) * hue_table[idx0] + frac * hue_table[idx1]);
        hue = map_16(base_hue, 0, 255, g->hue_base, g->hue_bound);
    }
    return hue;
}


void render_pattern(int index, uint8_t *framebuffer, int loop) {
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
    float anim = twopi * ((float)(curtime) / tau);

    for (int i = 0; i < LED_COUNT; i++) {
        // ---- HUE calculation ----
        uint8_t hue = calculate_pattern_hue(g, i, loop);

        // ---- VALUE (brightness sinusoid) ----
        float t = (float)i / (float)(LED_COUNT - 1);
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
            // Scale brightness by smooth_dB_brightness_level and effective_brightness
            // also set a minimum to prevent low light level flickering effect (slightly lower than the vu meter minimum of 0.2)
            val = (uint8_t)(fmaxf(smooth_dB_brightness_level, 0.2f) * effective_brightness * (val / 255.0f));
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
    flash_active = true;
    flash_end_time = esp_timer_get_time() / 1000 + 125; // 125ms from now
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
        if (flash_active) {
            // Render flash pattern
            for (int i = 0; i < LED_COUNT; i++) {
                set_pixel(framebuffer, i, 50, 50, 50);
            }
            update_leds(framebuffer);

            // Check if flash duration has passed
            int64_t now = esp_timer_get_time() / 1000;
            if (now >= flash_end_time) {
                flash_active = false;
            }
        } else {
            // Normal rendering
            if (force_safety_pattern) {
                safety_pattern(framebuffer);
            } else {
                render_pattern(settings.pattern_id, framebuffer, loop);
            }
            update_leds(framebuffer);

            loop = (loop + 1) % 256;
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}