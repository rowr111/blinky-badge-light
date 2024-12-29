#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <stdint.h>

// Function prototypes
void init_touch();
int get_touch_event(int pad_num);

// Touch event definitions
#define NO_TOUCH 0
#define SHORT_PRESS 1
#define LONG_PRESS 2

// Number of touch pads
#define NUM_TOUCH_PADS 3

// Touch pad GPIO assignments (adjust as needed)
#define TOUCH_PAD_1 17
#define TOUCH_PAD_2 18
#define TOUCH_PAD_3 19

#endif // TOUCH_INPUT_H