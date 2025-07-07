#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <stdint.h>

void init_touch();
bool get_touch_event(int pad_num);
void touch_debug_task(void *pvParameter);
void periodic_touch_recalibration_task(void *pvParameter);
void touch_task(void *param);

#endif // TOUCH_INPUT_H