#ifndef VU_METER_H
#define VU_METER_H

#include <stdint.h>
#include "genes.h"

void render_vu_meter_pattern(uint8_t *framebuffer, const genome *g, int loop);

#endif // VU_METER_H