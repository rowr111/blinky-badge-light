#ifndef PORTAL_H
#define PORTAL_H

#include "esp_err.h"

esp_err_t portal_http_start(void);
void portal_http_stop(void);

#endif // PORTAL_H
