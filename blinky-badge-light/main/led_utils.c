#include <math.h>
#include "led_utils.h"

Color Wheel(uint8_t wheelPos) {
    Color c;
    if (wheelPos < 85) {
        c.r = wheelPos * 3;
        c.g = 255 - wheelPos * 3;
        c.b = 0;
    } else if (wheelPos < 170) {
        wheelPos -= 85;
        c.r = 255 - wheelPos * 3;
        c.g = 0;
        c.b = wheelPos * 3;
    } else {
        wheelPos -= 170;
        c.r = 0;
        c.g = wheelPos * 3;
        c.b = 255 - wheelPos * 3;
    }
    return c;
}

void set_pixel(uint8_t *framebuffer, int index, uint8_t r, uint8_t g, uint8_t b) {
    framebuffer[index * 3 + 0] = g; // Green channel
    framebuffer[index * 3 + 1] = r; // Red channel
    framebuffer[index * 3 + 2] = b; // Blue channel
}

// Function to convert HSV to RGB
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float hf = h / 256.0f * 360.0f;
    if (hf >= 360.0f) hf -= 360.0f; // Ensure hue is in [0, 360)
    float sf = s / 255.0f;
    float vf = v / 255.0f;

    int i = (int)floor(hf / 60.0f) % 6;
    float f = hf / 60.0f - i;
    float p = vf * (1.0f - sf);
    float q = vf * (1.0f - f * sf);
    float t = vf * (1.0f - (1.0f - f) * sf);

    switch (i) {
        case 0:
            *r = vf * 255;
            *g = t * 255;
            *b = p * 255;
            break;
        case 1:
            *r = q * 255;
            *g = vf * 255;
            *b = p * 255;
            break;
        case 2:
            *r = p * 255;
            *g = vf * 255;
            *b = t * 255;
            break;
        case 3:
            *r = p * 255;
            *g = q * 255;
            *b = vf * 255;
            break;
        case 4:
            *r = t * 255;
            *g = p * 255;
            *b = vf * 255;
            break;
        case 5:
            *r = vf * 255;
            *g = p * 255;
            *b = q * 255;
            break;
    }
}    


// Simple map utility for 16-bit linear mapping
int16_t map_16(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    return (int16_t)(((int32_t)(x - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min);
}

// Clamp value between 0–255
uint8_t clamp8(int x) { return (x < 0) ? 0 : (x > 255) ? 255 : x; }


uint8_t satadd_8(uint8_t a, uint8_t b) {
    int16_t res = (int16_t)a + (int16_t)b;
    return (res > 255) ? 255 : (uint8_t)res;
}

uint8_t satsub_8(uint8_t a, uint8_t b) {
    int16_t res = (int16_t)a - (int16_t)b;
    return (res < 0) ? 0 : (uint8_t)res;
}