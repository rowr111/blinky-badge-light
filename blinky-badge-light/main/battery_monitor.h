#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

// Function declarations
void init_battery_monitor(void);
void battery_monitor_task(void *param);
void cleanup_battery_monitor();  // Clean up ADC resources

// Constants
#define VOLTAGE_DIVIDER_RATIO 1.4545 // Adjust based on resistors used (using 10k and 22k during testing)
#define ADC_ATTEN   ADC_ATTEN_DB_12
#define ADC_UNIT    ADC_UNIT_1

#define MOSFET_GATE_PIN GPIO_NUM_25
#define BUTTON_PIN GPIO_NUM_0

#define BRIGHT_THRESH     3550 // Brightness limiting threshold in mV
#define SAFETY_THRESH     3470 // Safety mode threshold in mV
#define OFF_THRESH   3330 // Turn off mode threshold in mV

extern volatile bool limit_brightness; // Flag to limit brightness when battery is low but not critical
extern volatile bool force_safety_pattern; // Flag to force safety pattern when battery is critically low

#endif // BATTERY_MONITOR_H
