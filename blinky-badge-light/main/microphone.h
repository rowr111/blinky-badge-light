#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdint.h>

void init_microphone(void);
void microphone_task(void *param);

#endif // MICROPHONE_H