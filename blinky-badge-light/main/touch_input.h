#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <stdint.h>

// Function prototypes
void init_touch();
int get_touch_event(int pad_num);
void touch_debug_task(void *pvParameter);
void periodic_touch_recalibration_task(void *pvParameter);

// Touch event definitions
#define NO_TOUCH 0
#define SHORT_PRESS 1
#define LONG_PRESS 2

#endif // TOUCH_INPUT_H