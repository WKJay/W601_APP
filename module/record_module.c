/*************************************************
Copyright (c) 2019
All rights reserved.
File name:     record_module.c
Description:   
History:
1. Version:    V1.0.0
Date:      2020-04-25
Author:    WKJay
Modify:    
*************************************************/
#include <rtthread.h>
#include <time.h>
#include <dfs_posix.h>
#include <time.h>
#include "W601_app.h"

#define DBG_TAG "record"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define DATA_RECORD_ROOT_PATH "/webnet/data"
#define DATA_RECORD_HEAD "time,temp,humi,used_mem,light\r\n"
#define DATA_FILTER_TIMES 60
#define DATA_RECORD_INTERVAL 1000 * 60 * 10   //10min

typedef struct _reacord_data
{
    float temp_sum;
    float humi_sum;
    float light_sum;
    uint16_t used_mem_sum;
    float temp_aver;
    float humi_aver;
    float light_aver;
    uint16_t used_mem_aver;
} record_data_t;

typedef struct _record
{
    rt_mutex_t file_mutex;
    struct tm cur_tm;
    char path[100];
    record_data_t data;
} record_t;

record_t record;

static int check_time_valid(void)
{
    LOG_D("start to check time validity...");
    while (1)
    {
        time_t cur_time = time(NULL);
        struct tm *local_time = localtime(&cur_time);
        if (local_time->tm_yday > 100)
        {
            LOG_D("time validated: %s", ctime(&cur_time));
            break;
        }
        rt_thread_mdelay(500);
    }
    return 0;
}

static void _module_init(void)
{
    DIR *dir = NULL;
    rt_memset(&record, 0, sizeof(record_t));
    record.file_mutex = rt_mutex_create("record", RT_IPC_FLAG_FIFO);
    //检测并生成文件目录
    dir = opendir(DATA_RECORD_ROOT_PATH);
    if (dir == NULL)
    {
        mkdir(DATA_RECORD_ROOT_PATH, 0x777);
        LOG_D("create dir: %s", DATA_RECORD_ROOT_PATH);
    }
    else
    {
        closedir(dir);
    }
}

/**
* Name:    dir_change
* Brief:   目录更换
* Input:   
* Return:  更换：1 未更换：0
*/
static int dir_change(void)
{
    DIR *dir = NULL;
    time_t cur_time = time(NULL);
    struct tm *local_time = localtime(&cur_time);
    //日期变化说明要更换目录
    if (local_time->tm_mday != record.cur_tm.tm_mday)
    {
        memset(record.path, 0, sizeof(record.path));
        //年份目录
        sprintf(record.path, "%s/%d", DATA_RECORD_ROOT_PATH, (local_time->tm_year + 1900));
        dir = opendir(record.path);
        if (dir == NULL)
        {
            mkdir(record.path, 0x777);
            LOG_D("create dir: %s", record.path);
        }
        else
        {
            closedir(dir);
        }
        //月份目录
        sprintf(record.path, "%s/%d", record.path, (local_time->tm_mon + 1));
        dir = opendir(record.path);
        if (dir == NULL)
        {
            mkdir(record.path, 0x777);
            LOG_D("create dir: %s", record.path);
        }
        else
        {
            closedir(dir);
        }
        
        memcpy(&record.cur_tm,local_time,sizeof(struct tm));
        return 1;
    }
    else
    {
        return 0;
    }
}

static int data_record(void)
{
    int fd = NULL;
    char record_data[200];
    unsigned int cur_time = 0;

    //只有当目录变更时才重新组织目录
    if (dir_change() == 1)
    {
        sprintf(record.path, "%s/%d.csv", record.path, record.cur_tm.tm_mday);
    }
    cur_time = time(NULL);
    fd = open(record.path, O_RDONLY);
    //第一次打开，写入数据头
    if (fd < 0)
    {
        LOG_D("create file %s", record.path);
        fd = open(record.path, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd)
        {
            write(fd, DATA_RECORD_HEAD, strlen(DATA_RECORD_HEAD));
            close(fd);
        }
        else
        {
            LOG_E("create file %s failed", record.path);
            return -1;
        }
    }
    else
    {
        close(fd);
    }

    //写入数据
    fd = open(record.path, O_WRONLY | O_CREAT | O_APPEND);
    memset(record_data, 0, sizeof(record_data));
    //时间戳-温度-湿度-内存-光照
    snprintf(record_data, sizeof(record_data), "%d,%.2f,%.2f,%d,%.2f\r\n", cur_time,
             record.data.temp_aver,
             record.data.humi_aver,
             record.data.used_mem_aver,
             record.data.light_aver);

    write(fd, record_data, strlen(record_data));
    close(fd);
    //LOG_D("Record: %s", record_data);
    return 0;
}

static void record_thread_entry(void *param)
{

    while (1)
    {
        record.data.humi_sum = 0;
        record.data.light_sum = 0;
        record.data.temp_sum = 0;
        record.data.used_mem_sum = 0;
        for (int i = 0; i < DATA_FILTER_TIMES; i++)
        {
            record.data.humi_sum += w601.aht10_data.cur_humi;
            record.data.temp_sum += w601.aht10_data.cur_temp;
            record.data.light_sum += w601.ap3216c_data.cur_light;
            record.data.used_mem_sum += used_mem;
            rt_thread_mdelay(DATA_RECORD_INTERVAL / DATA_FILTER_TIMES);
        }
        record.data.humi_aver = record.data.humi_sum / DATA_FILTER_TIMES;
        record.data.temp_aver = record.data.temp_sum / DATA_FILTER_TIMES;
        record.data.light_aver = record.data.light_sum / DATA_FILTER_TIMES;
        record.data.used_mem_aver = record.data.used_mem_sum / DATA_FILTER_TIMES;

        rt_mutex_take(record.file_mutex, 6000);
        data_record();
        rt_mutex_release(record.file_mutex);
    }
}

int record_module_init(void)
{
    _module_init();
    check_time_valid();
    rt_thread_t record_tid = rt_thread_create("record", record_thread_entry, NULL, 4096, 10, 5);
    if (record_tid)
    {
        rt_thread_startup(record_tid);
        return 0;
    }
    else
    {
        LOG_E("create record thread failed");
        return -1;
    }
}
