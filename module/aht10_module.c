/*************************************************
 Copyright (c) 2019
 All rights reserved.
 File name:     aht10_module.c
 Description:   aht10模块源文件
 History:
 1. Version:    V1.0.0
    Date:       2019-11-30
    Author:     WKJay
    Modify:     新建
*************************************************/
#include <stdio.h>
#include "rtthread.h"
#include "rtdevice.h"
#include "aht10_module.h"
#include "cJSON.h"
#include "sensor_asair_aht10.h"

#define DBG_TAG "aht10"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define TEMP_DEV "temp_aht"
#define HUMI_DEV "humi_aht"

static rt_device_t temp_dev, humi_dev;
static struct rt_sensor_data temp_dev_data, humi_dev_data;
w601_aht10_t w601_aht10;

/**
 * Name:    aht10_device_init
 * Brief:   初始化aht10
 * Input:   None
 * Return:  
 * success:0
 * fail:-1
 */
int aht10_device_init(void)
{
    LOG_D("Temperature and Humidity Sensor initialize start...");
    rt_thread_mdelay(2000);

    rt_memset(&w601_aht10, 0, sizeof(w601_aht10_t));
    //获取温度设备句柄
    temp_dev = rt_device_find(TEMP_DEV);
    if (temp_dev == RT_NULL)
    {
        LOG_E("can not find TEMP device: %s", TEMP_DEV);
        return -1;
    }
    else
    {
        if (rt_device_open(temp_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open TEMP device failed!");
            return -1;
        }
    }

    //获取湿度设备句柄
    humi_dev = rt_device_find(HUMI_DEV);
    if (humi_dev == RT_NULL)
    {
        LOG_E("can not find HUMI device: %s", HUMI_DEV);
        return RT_ERROR;
    }
    else
    {
        if (rt_device_open(humi_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open HUMI device failed!");
            return RT_ERROR;
        }
    }

    LOG_I("Temperature and Humidity Sensor initialize success !");
    return 0;
}

/**
 * Name:    aht10_temp_get
 * Brief:   aht10模块温度获取
 * Input:   None
 * Return:  当前读取的温度
 */
static float aht10_temp_get(void)
{
    float temp_data = 0;
    //获取数据
    rt_device_read(temp_dev, 0, &temp_dev_data, 1);
    //组织数据
    temp_data = (int)(temp_dev_data.data.temp / 10) + ((float)(temp_dev_data.data.temp % 10) / 10);

    return temp_data;
}

/**
 * Name:    aht10_humi_get
 * Brief:   aht10模块湿度获取
 * Input:   None
 * Return:  当前读取的湿度
 */
static float aht10_humi_get(void)
{
    float humi_data = 0;
    rt_device_read(humi_dev, 0, &humi_dev_data, 1);

    //组织数据
    humi_data = (int)(humi_dev_data.data.humi / 10) + ((float)(humi_dev_data.data.humi % 10) / 10);

    return humi_data;
}

/**
* Name:    aht10_thread_entry
* Brief:   aht10线程主体
* Input:   None
* Return:  None 
*/
static void aht10_thread_entry(void *param)
{
    int sec_count = 3600; //时间计数
    //aht10初始化
    aht10_device_init();

    while (1)
    {
        w601_aht10.cur_temp = aht10_temp_get();
        w601_aht10.cur_humi = aht10_humi_get();

        //LOG_D("temp: %d.%d *C --- humi: %d.%d %%", (int)temp, (int)(temp * 10) % 10, (int)humi, (int)(humi * 10) % 10);

        //一个小时
        if (sec_count == 3600)
        {
            sec_count = 1;

            //湿度
            if (w601_aht10.cur_humi_index < 23)
            {
                w601_aht10.humi_data[w601_aht10.cur_humi_index++] = w601_aht10.cur_humi;
            }
            else
            {
                for (int i = 1; i <= 23; i++)
                {
                    w601_aht10.humi_data[i - 1] = w601_aht10.humi_data[i];
                }
                w601_aht10.humi_data[23] = w601_aht10.cur_humi;
            }

            //温度
            if (w601_aht10.cur_temp_index < 23)
            {
                w601_aht10.temp_data[w601_aht10.cur_temp_index++] = w601_aht10.cur_temp;
            }
            else
            {
                for (int i = 1; i <= 23; i++)
                {
                    w601_aht10.temp_data[i - 1] = w601_aht10.temp_data[i];
                }
                w601_aht10.temp_data[23] = w601_aht10.cur_temp;
            }
        }

        sec_count++;
        rt_thread_mdelay(1000);
    }
}

/**
* Name:    aht10_temp_data_get
* Brief:   温度数据读取
* Input:   
* Return:  aht10结构体句柄
*/
w601_aht10_t *aht10_data_get(void)
{
    return &w601_aht10;
}

char *json_create_aht10_current_data(void)
{
    char *json_data = RT_NULL;
    char value[10] = "";
    cJSON *root = cJSON_CreateObject();
    snprintf(value, sizeof(value), "%.2f", w601_aht10.cur_temp);
    cJSON_AddItemToObject(root, "temp", cJSON_CreateString(value));

    snprintf(value, sizeof(value), "%.2f", w601_aht10.cur_humi);
    cJSON_AddItemToObject(root, "humi", cJSON_CreateString(value));
    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}

char *json_create_aht10_saved_data(void)
{
    char *json_data = RT_NULL;
    char value[10] = "";

    cJSON *root = cJSON_CreateObject();
    cJSON *temp = cJSON_CreateArray();
    cJSON *humi = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "temp", temp);
    cJSON_AddItemToObject(root, "humi", humi);

    for (int i = 0; i < w601_aht10.cur_humi_index; i++)
    {
        snprintf(value, sizeof(value), "%.2f", w601_aht10.humi_data[i]);
        cJSON_AddItemToObject(humi, "humi", cJSON_CreateString(value));
    }

    for (int i = 0; i < w601_aht10.cur_temp_index; i++)
    {
        snprintf(value, sizeof(value), "%.2f", w601_aht10.temp_data[i]);
        cJSON_AddItemToObject(temp, "temp", cJSON_CreateString(value));
    }
    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}
/**
* Name:    aht10_module_init
* Brief:   aht10模块初始化
* Input:   None
* Return:  Success: 0   Fail: -1
*/
int aht10_module_init(void)
{
    rt_thread_t aht10_tid = RT_NULL;
    aht10_tid = rt_thread_create("aht10_t", aht10_thread_entry,
                                 NULL, 1024, 15, 5);
    if (!aht10_tid)
    {
        LOG_E("aht10 thread create fail");
        return -1;
    }
    else
    {
        rt_thread_startup(aht10_tid);
    }

    return 0;
}
