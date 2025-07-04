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

// Order LEDs from bottom tip up both sides to top center
static const uint8_t heart_fill_order[13][2] = {
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


void set_pixel(uint8_t *framebuffer, int index, uint8_t r, uint8_t g, uint8_t b);
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
int16_t map_16(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max);
uint8_t clamp8(int x);
uint8_t satadd_8(uint8_t a, uint8_t b);
uint8_t satsub_8(uint8_t a, uint8_t b);

#endif // LED_UTILS_H