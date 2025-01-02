#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>

// Function declarations
void init_battery_monitor(void);
void battery_monitor_task(void *param);
void cleanup_battery_monitor();  // Clean up ADC resources

// Constants
#define ADC_CHANNEL ADC_CHANNEL_0  // GPIO36
#define VOLTAGE_DIVIDER_RATIO 1.4545 // Adjust based on resistors used (using 10k and 22k during testing)

#define BRIGHT_THRESH     3550 // Brightness limiting threshold in mV
#define SAFETY_THRESH     3470 // Safety mode threshold in mV
#define SHIPMODE_THRESH   3330 // Ship mode threshold in mV

#endif // BATTERY_MONITOR_H
