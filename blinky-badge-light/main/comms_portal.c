#include <stdio.h>
#include "esp_random.h"
#include "wifi_portal.h"
#include "portal.h"
#include "dns_portal.h"

void now_init(void);

void comms_portal_start(void)
{
    // Bring up Wi-Fi AP+STA on a fixed channel (e.g., 6)
    portal_wifi_cfg_t w = {0};
    snprintf(w.ssid, sizeof(w.ssid), "BlinkyBadge_%04X",
             (unsigned)(esp_random() & 0xFFFF));
    // Open AP for now (leave password empty or set an 8+ char WPA2 key)
    w.password[0] = '\0';
    w.channel = 6;
    portal_wifi_start(&w);

    // Start DNS portal (point all names to 192.168.4.1)
    dns_portal_start( (192u) | (168u<<8) | (4u<<16) | (1u<<24) );

    // Initialize ESP-NOW using the already-running Wi-Fi
    now_init();

    // Serve a static form at "/"
    portal_http_start();
}
