#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include "esp_err.h"
#include <stdint.h>

typedef struct {
    char ssid[32];
    char password[64];   // empty = open AP (WPA2 if 8+ chars)
    uint8_t channel;     // keep fixed; e.g., 1/6/11
} portal_wifi_cfg_t;

esp_err_t portal_wifi_start(const portal_wifi_cfg_t *cfg);
void portal_wifi_stop(void);

#endif // WIFI_PORTAL_H
