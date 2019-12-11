#ifndef _WIFI_MODULE_H
#define _WIFI_MODULE_H
#include <stdint.h>

int wifi_start(void);
uint8_t wifi_ready_status_get(void);

#endif
