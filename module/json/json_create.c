/*************************************************
Copyright (c) 2019
All rights reserved.
File name:     json_create.c
Description:   
History:
1. Version:    V1.0.0
Date:      2020-04-25
Author:    WKJay
Modify:    
*************************************************/
#include <stdio.h>
#include "W601_app.h"
#include "cJSON.h"

#define LOG_TAG "json"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

/**
 * Name:    json_create_web_response
 * Brief:   创建一个web响应
 * Input:   
 *  @code:  响应码
 *  @msg:   响应信息
 * Output:  响应体字符串
 */
char *json_create_web_response(int code, const char *msg)
{
    char *json_data = NULL;
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    if (root == NULL)
    {
        LOG_E("create web response json object error!");
        return 0;
    }

    if (msg == NULL)
    {
        msg = " ";
    }

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(code));
    cJSON_AddItemToObject(root, "msg", cJSON_CreateString(msg));

    json_data = cJSON_PrintUnformatted(root);
    if (root)
    {
        cJSON_Delete(root);
    }

    return json_data;
}

/**
* Name:    json_create_info
* Brief:   基本信息json字符串
* Input:   
* Return:  Success: 0   Fail: -1
*/
char *json_create_info(void)
{
    char *json_data = NULL;
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    char data_str[10];
    uint32_t used_mem, max_mem, total_mem;

    if (root == NULL)
    {
        LOG_E("create web response json object error!");
        return 0;
    }
    
    rt_memory_info(&total_mem, &used_mem, &max_mem);
    cJSON_AddItemToObject(root, "used_mem", cJSON_CreateNumber(used_mem));
    snprintf(data_str, sizeof(data_str), "%.2f", w601.aht10_data.cur_humi);
    cJSON_AddItemToObject(root, "humi", cJSON_CreateString(data_str));
    snprintf(data_str, sizeof(data_str), "%.2f", w601.aht10_data.cur_temp);
    cJSON_AddItemToObject(root, "temp", cJSON_CreateString(data_str));
    snprintf(data_str, sizeof(data_str), "%.2f", w601.ap3216c_data.cur_light);
    cJSON_AddItemToObject(root, "light", cJSON_CreateString(data_str));
    cJSON_AddItemToObject(root, "ts", cJSON_CreateNumber((unsigned int)time(NULL)));

    json_data = cJSON_PrintUnformatted(root);
    if (root)
    {
        cJSON_Delete(root);
    }

    return json_data;
}
