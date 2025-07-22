#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "firework_notification_pattern.h"
#include "led_utils.h" // for set_pixel, hsv_to_rgb, etc.
#include "pins.h"      // for LED_COUNT

volatile bool show_firework_notification = false;
int firework_notification_start_time = 0;

void render_firework_notification_pattern(uint8_t *framebuffer, int elapsed_ms) {
    // Phase 1: White swell (first 1000 ms)
    const int swell_time = 1000;
    if (elapsed_ms < swell_time) {
        float frac = (float)elapsed_ms / swell_time;
        uint8_t bright = (uint8_t)(frac * 255);
        for (int i = 0; i < LED_COUNT; i++) {
            set_pixel(framebuffer, i, bright, bright, bright);
        }
        return;
    }

    // Phase 2: Strobing random rainbow, fading out (rest of the time)
    int strobe_phase_len = 60;  // ms per strobe
    int fade_time = FIREWORK_NOTIFICATION_TOTAL_MS - swell_time;
    int fade_elapsed = elapsed_ms - swell_time;
    float fade = 1.0f - ((float)fade_elapsed / fade_time);
    if (fade < 0) fade = 0;

    // Figure out strobe state
    int strobe_count = fade_elapsed / strobe_phase_len;
    bool strobe_on = (strobe_count % 2) == 0;

    if (strobe_on) {
        for (int i = 0; i < LED_COUNT; i++) {
            // Each LED: 1-in-3 chance to light up in this strobe phase
            // Use a "stable" but strobe-unique pseudo-random: combine i and strobe_count
            unsigned int prand = (i * 167 + strobe_count * 73) ^ (strobe_count * 311);
            if ((prand % 3) == 0) {
                // Light this LED: random hue, full sat, fade-dimmed value
                uint8_t hue = (uint8_t)(prand % 255);
                uint8_t r, g, b;
                hsv_to_rgb(hue, 255, (uint8_t)(fade * 255), &r, &g, &b);
                set_pixel(framebuffer, i, r, g, b);
            } else {
                // Off
                set_pixel(framebuffer, i, 0, 0, 0);
            }
        }
    } else {
        // All off between strobes
        for (int i = 0; i < LED_COUNT; i++) {
            set_pixel(framebuffer, i, 0, 0, 0);
        }
    }
}
