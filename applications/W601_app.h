#ifndef __W601_APP_H
#define __W601_APP_H
#include "rtthread.h"
#include "sensor.h"
#include "w601_module.h"

#define SOFTWARE_VERSION "W601V1.2"
extern rt_size_t used_mem, max_mem, mem_size_aligned;

typedef struct _w601_mutex {
    rt_mutex_t sensor_mutex;
} w601_mutex_t;

typedef struct _w601_rgb {
    uint8_t red : 1;
    uint8_t blue : 1;
    uint8_t green : 1;
    uint8_t reserve : 5;
} w601_rgb_t;

typedef struct _w601 {
    w601_mutex_t mutex;
    w601_aht10_t aht10_data;
    w601_ap3216c_t ap3216c_data;
    w601_rgb_t rgb;
} w601_t;

extern w601_t w601;
extern time_t ntp_sync_to_rtc(const char *host_name);
// w601应用初始化
void w601_app_init(void);

#endif
