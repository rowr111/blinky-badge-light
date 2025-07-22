#ifndef BLE_H
#define BLE_H


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ble_init(void);

void ble_scan_task(void *param);
void ble_advertise_task(void *param);

extern TaskHandle_t ble_adv_task_handle;

#endif // BLE_H