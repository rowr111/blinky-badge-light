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
    float hf = h / 255.0f * 360.0f;
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