#include <math.h>
#include "genes.h"
#include "led_utils.h"
#include "microphone.h"
#include "pins.h"
#include "led_control.h"
#include "esp_log.h"

// Example: Order LEDs from bottom tip up both sides to top center
static const uint8_t vu_order[13][2] = {
    {12, 12},
    {11, 13}, 
    {10, 14},
    {9, 15},
    {8, 16},
    {7, 17},
    {6, 18},
    {5, 19},    
    {4, 20},
    {3, 21},
    {2, 22},
    {1, 23},
    {0, 0},   
};

static float vu_display_level = 0.0f;
#define VU_ATTACK_RATE 0.25f  // How fast it can rise
#define VU_DECAY_RATE 0.02f   // How slow it falls

void render_vu_meter_pattern(uint8_t *framebuffer, const genome *g, int loop) {
    if (smooth_dB_brightness_level > vu_display_level) {
        vu_display_level += VU_ATTACK_RATE * (smooth_dB_brightness_level - vu_display_level);
    } else {
        vu_display_level -= VU_DECAY_RATE;
        if (vu_display_level < smooth_dB_brightness_level)
            vu_display_level = smooth_dB_brightness_level;
        if (vu_display_level < 0.0f) vu_display_level = 0.0f;
    }

    int levels = sizeof(vu_order) / sizeof(vu_order[0]);
    int num_lit_levels = (int)ceilf(vu_display_level * levels);

    if (num_lit_levels < 0) num_lit_levels = 0;
    if (num_lit_levels > levels) num_lit_levels = levels;

    float global_brightness = brightness / 255.0f;

    // Uncomment this block to draw base color at 20% brightness on all LEDs
    // 1. Draw base color at 20% brightness on all LEDs
    //for (int i = 0; i < LED_COUNT; i++) {
    //    uint8_t hue = (g->hue_base + i * g->hue_ratedir + loop) % 256;
    //    Color color = Wheel(hue);
    //    float sat = g->sat / 255.0f;
    //    float base_brightness = 0.1f * global_brightness; // 10% of global brightness
    //    set_pixel(framebuffer, i,
    //              (uint8_t)(color.r * sat * base_brightness),
    //              (uint8_t)(color.g * sat * base_brightness),
    //              (uint8_t)(color.b * sat * base_brightness));
    //}

    // for (int lvl = 0; lvl < levels; lvl++) {
    //     if (lvl < num_lit_levels) {
    //         uint8_t hue = (g->hue_base + lvl * g->hue_ratedir + loop) % 256;
    //         Color color = Wheel(hue);
    //         float sat = g->sat / 255.0f;
    //         float per_level = (levels > 1) ? (0.7f + 0.3f * ((float)lvl / (levels - 1))) : 1.0f;
    //         float pos_brightness = per_level * smooth_dB_brightness_level;
    //         if (pos_brightness < 0.2f) pos_brightness = 0.2f;
    //         if (pos_brightness > 1.0f) pos_brightness = 1.0f;

    //         uint8_t r = (uint8_t)(color.r * sat * pos_brightness * global_brightness);
    //         uint8_t g_col = (uint8_t)(color.g * sat * pos_brightness * global_brightness);
    //         uint8_t b = (uint8_t)(color.b * sat * pos_brightness * global_brightness);

    //         set_pixel(framebuffer, vu_order[lvl][0], r, g_col, b);
    //         if (vu_order[lvl][1] != vu_order[lvl][0]) {
    //             set_pixel(framebuffer, vu_order[lvl][1], r, g_col, b);
    //         }
    //     }
    // }

    // OG clean idea just draw the lit levels
    for (int lvl = 0; lvl < levels; lvl++) {
        uint8_t r = 0, g_col = 0, b = 0;
        if (lvl < num_lit_levels) {
            uint8_t hue = (g->hue_base + lvl * g->hue_rate + loop) % 256;
            Color color = Wheel(hue);
            float sat = g->sat / 255.0f;
            float per_level = (levels > 1) ? (0.7f + 0.3f * ((float)lvl / (levels - 1))) : 1.0f;
            float pos_brightness = per_level * smooth_dB_brightness_level;  // (or dB_brightness_level, depending on your choice)
            if (pos_brightness < 0.2f) pos_brightness = 0.2f;
            if (pos_brightness > 1.0f) pos_brightness = 1.0f;

            r = (uint8_t)(color.r * sat * pos_brightness * global_brightness);
            g_col = (uint8_t)(color.g * sat * pos_brightness * global_brightness);
            b = (uint8_t)(color.b * sat * pos_brightness * global_brightness);
        }
        set_pixel(framebuffer, vu_order[lvl][0], r, g_col, b);
        if (vu_order[lvl][1] != vu_order[lvl][0]) {
            set_pixel(framebuffer, vu_order[lvl][1], r, g_col, b);
        }
    }

}