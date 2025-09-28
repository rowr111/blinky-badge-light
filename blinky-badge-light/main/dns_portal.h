#ifndef DNS_PORTAL_H
#define DNS_PORTAL_H

#include "esp_err.h"

esp_err_t dns_portal_start(uint32_t ap_ip); // e.g., 0x0104A8C0 for 192.168.4.1 (in little-endian use IPADDR4_INIT)
void dns_portal_stop(void);

#endif // DNS_PORTAL_H
