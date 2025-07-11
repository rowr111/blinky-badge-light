#include "battery_monitor.h"
#include "battery_level_pattern.h"
#include "led_control.h"
#include "led_utils.h"
#include "pins.h"

volatile bool show_battery_meter = false;
int battery_meter_start_time = 0;


void render_battery_level_pattern(uint8_t *framebuffer, int elapsed_ms) {
    int levels = 13;

    // Calculate battery fill fraction (0.0 to 1.0)
    float battery_frac = (float)(current_battery_voltage - OFF_THRESH) / (MAX_BATTERY_VOLTAGE - OFF_THRESH);
    if (battery_frac < 0.0f) battery_frac = 0.0f;
    if (battery_frac > 1.0f) battery_frac = 1.0f;

    // Animate fill for first 2 seconds, then hold
    float fill_progress;
    if (elapsed_ms < BATTERY_FILL_ANIM_MS) {
        fill_progress = battery_frac * ((float)elapsed_ms / BATTERY_FILL_ANIM_MS);
    } else {
        fill_progress = battery_frac;
    }

    for (int lvl = 0; lvl < levels; lvl++) {
        uint8_t s = 255, v = effective_brightness;
        uint8_t h = 0;
        uint8_t r, g, b;
        
        float led_end_frac = (float)lvl / (float)levels;
        if (fill_progress >= led_end_frac) {
            float frac = (float)lvl / (levels - 1);
            h = (uint8_t)(frac * 85.0f); // Gradient from red (0) to green (85)
        } else {
            v = 0; // Off
        }
        hsv_to_rgb(h, s, v, &r, &g, &b);
        set_pixel(framebuffer, heart_fill_order[lvl][0], r, g, b);
        if (heart_fill_order[lvl][1] != heart_fill_order[lvl][0]) {
            set_pixel(framebuffer, heart_fill_order[lvl][1], r, g, b);
        }
    }
}

