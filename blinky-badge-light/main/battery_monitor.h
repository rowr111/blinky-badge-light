#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <driver/gpio.h>

// Function declarations
void init_battery_monitor(void);
void battery_monitor_task(void *param);
void cleanup_battery_monitor();  // Clean up ADC resources
void turn_off(void);

// Constants
#define VOLTAGE_DIVIDER_RATIO 2 // Using two 10k resistors
#define ADC_ATTEN   ADC_ATTEN_DB_12
#define ADC_UNIT    ADC_UNIT_1

#define BRIGHT_THRESH     3550 // Brightness limiting threshold in mV
#define SAFETY_THRESH     3470 // Safety mode threshold in mV
#define OFF_THRESH        3330 // Turn off mode threshold in mV

extern volatile bool limit_brightness; // Flag to limit brightness when battery is low but not critical
extern volatile bool force_safety_pattern; // Flag to force safety pattern when battery is critically low
extern volatile uint16_t current_battery_voltage; // Current battery voltage in mV

#endif // BATTERY_MONITOR_H
