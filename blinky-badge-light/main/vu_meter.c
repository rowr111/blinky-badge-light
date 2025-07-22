#include <math.h>
#include "genes.h"
#include "led_utils.h"
#include "microphone.h"
#include "pins.h"
#include "led_control.h"
#include "esp_log.h"

static float vu_display_level = 0.0f;
#define VU_ATTACK_RATE 0.25f  // How fast it can rise
#define VU_DECAY_RATE 0.02f   // How slow it falls

static inline uint8_t scale_channel(uint8_t c, float scale) {
    return (uint8_t)(c * scale);
}

void render_vu_meter_pattern(uint8_t *framebuffer, const genome *g, int loop) {
    if (smooth_dB_brightness_level > vu_display_level) {
        vu_display_level += VU_ATTACK_RATE * (smooth_dB_brightness_level - vu_display_level);
    } else {
        vu_display_level -= VU_DECAY_RATE;
        if (vu_display_level < smooth_dB_brightness_level)
            vu_display_level = smooth_dB_brightness_level;
        if (vu_display_level < 0.0f) vu_display_level = 0.0f;
    }

    int levels = sizeof(heart_fill_order) / sizeof(heart_fill_order[0]);
    int num_lit_levels = (int)ceilf(vu_display_level * levels);
    num_lit_levels = (int)fmaxf(0, fminf(num_lit_levels, levels));

    float global_brightness = effective_brightness / 255.0f;

    for (int lvl = 0; lvl < levels; lvl++) {
        int led_idx0 = heart_fill_order[lvl][0];
        int led_idx1 = heart_fill_order[lvl][1];
        uint8_t r0 = 0, g0 = 0, b0 = 0;

        if (lvl < num_lit_levels) {
            uint8_t hue0 = calculate_pattern_hue(g, led_idx0, loop);
            hsv_to_rgb(hue0, g->sat, 255, &r0, &g0, &b0);

            float per_level = (levels > 1) ? (0.7f + 0.3f * ((float)lvl / (levels - 1))) : 1.0f;
            float pos_brightness = fminf(fmaxf(per_level * smooth_dB_brightness_level, 0.2f), 1.0f);
            float scale = pos_brightness * global_brightness;

            r0 = scale_channel(r0, scale);
            g0 = scale_channel(g0, scale);
            b0 = scale_channel(b0, scale);
        }
        set_pixel(framebuffer, led_idx0, r0, g0, b0);
        if (led_idx1 != led_idx0) {
            set_pixel(framebuffer, led_idx1, r0, g0, b0);
        }
    }

}