#ifndef BATTERY_LEVEL_PATTERN_H
#define BATTERY_LEVEL_PATTERN_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool show_battery_meter;
extern int battery_meter_start_time;

// Duration constants (ms)
#define BATTERY_FILL_ANIM_MS 1000
#define BATTERY_HOLD_MS      3000
#define BATTERY_TOTAL_MS     (BATTERY_FILL_ANIM_MS + BATTERY_HOLD_MS)

void render_battery_level_pattern(uint8_t *framebuffer, int elapsed_ms, uint8_t effective_brightness);

#endif // BATTERY_LEVEL_PATTERN_H