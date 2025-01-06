#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdint.h>

extern volatile bool mic_active;

// Function Declarations
void init_microphone(void);
int get_sound_level(void); // Returns decibel level
void microphone_task(void *param);

// Constants
#define SAMPLE_RATE 16000  // Adjust sample rate as needed
#define I2S_NUM I2S_NUM_0  // Using I2S0 interface
#define SAMPLE_BUFFER_SIZE 512

#endif // MICROPHONE_H
