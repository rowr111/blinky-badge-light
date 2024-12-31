#ifndef LED_UTILS_H
#define LED_UTILS_H

#include <stdint.h>

// RGB Color Structure
typedef struct {
    uint8_t r; // Red component
    uint8_t g; // Green component
    uint8_t b; // Blue component
} Color;

Color Wheel(uint8_t wheelPos);

void set_pixel(uint8_t *framebuffer, int index, uint8_t r, uint8_t g, uint8_t b);
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

#endif // LED_UTILS_H