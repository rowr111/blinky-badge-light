#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdint.h>

extern volatile bool mic_active;

// Function Declarations
void init_microphone(void);
float get_sound_level(void); // Returns decibel level
void microphone_task(void *param);
void i2s_matrix_dump_task(void *param);

#endif // MICROPHONE_H