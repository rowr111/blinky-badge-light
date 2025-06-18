#include "genes.h"
#include "led_utils.h"
#include "microphone.h"

// Example: Order LEDs from bottom tip up both sides to top center
static const uint8_t vu_order[LED_COUNT/2 + 1][2] = {
    {0, 0},      // bottom tip
    {1, 23},  // up left, up right
    {2, 22},
    {3, 21},
    {4, 20},
    {5, 19},
    {6, 18},
    {7, 17},
    {8, 16},
    {9, 15},
    {10, 14},
    {11, 13},  
    {12, 12}
};


void render_vu_meter_pattern(uint8_t *framebuffer, const genome *g) {
    int levels = sizeof(vu_order) / sizeof(vu_order[0]);
    int num_lit_levels = (int)(dB_brightness_level * levels);
    if (num_lit_levels < 0) num_lit_levels = 0;
    if (num_lit_levels > levels) num_lit_levels = levels;

    for (int lvl = 0; lvl < levels; lvl++) {
        uint8_t r = 0, g_col = 0, b = 0;
        if (lvl < num_lit_levels) {
            uint8_t hue = (g->hue_base + lvl * g->hue_ratedir) % 256;
            Color color = Wheel(hue);
            float sat = g->sat / 255.0f;
            float brightness = g->val / 255.0f;
            float pos_brightness = brightness * (0.7f + 0.3f * ((float)lvl / (levels - 1)));
            r = (uint8_t)(color.r * sat * pos_brightness);
            g_col = (uint8_t)(color.g * sat * pos_brightness);
            b = (uint8_t)(color.b * sat * pos_brightness);
        }
        set_pixel(framebuffer, vu_order[lvl][0], r, g_col, b);
        if (vu_order[lvl][1] != vu_order[lvl][0]) {
            set_pixel(framebuffer, vu_order[lvl][1], r, g_col, b);
        }
    }
}