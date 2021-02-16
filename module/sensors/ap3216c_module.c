#include <rtthread.h>
#include <rtdevice.h>
#include "W601_app.h"
#include "sensor_lsc_ap3216c.h"

#define LOG_TAG "ap3216c"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

#define ALS_DEV "li_ap321"
#define PS_DEV "pr_ap321"

rt_device_t als_dev;
struct rt_sensor_data als_dev_data;

/**
 * Name:    ap3216c_als_get
 * Brief:   ap3216c
 * Input:   None
 * Return:  
 */
static float ap3216c_als_get(void)
{
    float als_data = 0;
    rt_mutex_take(w601.mutex.sensor_mutex, 1200);
    rt_device_read(als_dev, 0, &als_dev_data, 1);
    rt_mutex_release(w601.mutex.sensor_mutex);
    //组织数据
    als_data = (int)(als_dev_data.data.light / 10) + ((float)(als_dev_data.data.humi % 10) / 10.0);

    return als_data;
}

void ap3216c_thread(void *param)
{
    LOG_D("Als Ps Sensor  Start...");

    /* 查找并打开光强传感器 */
    als_dev = rt_device_find(ALS_DEV);
    if (als_dev == RT_NULL)
    {
        LOG_E("can not find ALS device: %s", ALS_DEV);
        return;
    }
    else
    {
        if (rt_device_open(als_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open ALS device failed!");
            return;
        }
    }

    /* 开始读取传感器数据 */
    while (1)
    {
        rt_mutex_take(w601.mutex.sensor_mutex, 1200);
        w601.ap3216c_data.cur_light = ap3216c_als_get();
        rt_mutex_release(w601.mutex.sensor_mutex);
        rt_thread_mdelay(500);
    }
}

int ap3216c_module_init(void)
{
    rt_memset(&w601.ap3216c_data, 0, sizeof(w601_ap3216c_t));
    rt_thread_t ap3216c_tid = RT_NULL;
    ap3216c_tid = rt_thread_create("ap3216c", ap3216c_thread,
                                   NULL, 1024, 15, 5);
    if (!ap3216c_tid)
    {
        LOG_E("ap3216c thread create fail");
        return -1;
    }
    else
    {
        rt_thread_startup(ap3216c_tid);
    }

    return 0;
}
