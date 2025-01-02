#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>

// Function declarations
void init_battery_monitor(void);
void battery_monitor_task(void *param);

// Constants
#define ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36
#define VOLTAGE_DIVIDER_RATIO 1.4545 // Adjust based on resistors used (using 10k and 22k during testing)

#endif // BATTERY_MONITOR_H
