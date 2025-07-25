#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <stdint.h>

void init_touch();
bool get_is_touched(int pad_num);
void touch_task(void *param);

#endif // TOUCH_INPUT_H