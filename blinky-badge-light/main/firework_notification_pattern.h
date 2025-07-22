#ifndef FIREWORK_NOTIFICATION_PATTERN_H
#define FIREWORK_NOTIFICATION_PATTERN_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool show_firework_notification;
extern int firework_notification_start_time;

// Duration constants (ms)
#define FIREWORK_NOTIFICATION_TOTAL_MS 5000 // Total duration of the firework notification

void render_firework_notification_pattern(uint8_t *framebuffer, int elapsed_ms);

#endif // FIREWORK_NOTIFICATION_PATTERN_H