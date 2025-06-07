#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <stdint.h>

extern volatile bool mic_active;

// Function Declarations
void init_microphone(void);
float get_sound_level(void); // Returns decibel level
void microphone_task(void *param);
void set_mic_active(bool active);

// Constants
#define SAMPLE_RATE 16000  // Adjust sample rate as needed
#define I2S_NUM I2S_NUM_0  // Using I2S0 interface
#define SAMPLE_BUFFER_SIZE 128

#define SAMPLE_BITS         I2S_DATA_BIT_WIDTH_24BIT  // Adjust based on microphone's bit width
#define CHANNEL_COUNT       1       // Mono
#define DMA_BUFFER_COUNT    4
#define DMA_BUFFER_SIZE     256     // Number of samples per buffer


#endif // MICROPHONE_H