/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     mqtt_json.c
 Description:
 History:
 1. Version:
    Date:       2021-02-14
    Author:     WKJay
    Modify:
*************************************************/
#include <rtthread.h>
#include <cjson.h>
#include <stdio.h>
#include "module_utils.h"
#include "mqtt_json.h"
#include "W601_app.h"

#define LOG_TAG "mqtt"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

int mqtt_cfg_parse(mqtt_cfg_t *cfg) {
    int rc = -1;
    char *json_data = RT_NULL;
    cJSON *root = RT_NULL;

    json_data = (char *)file_read(MQTT_CFG_PATH);
    if (json_data == RT_NULL) {
        return -1;
    }

    root = cJSON_Parse(json_data);
    if (root == RT_NULL) {
        LOG_E("parse json data failed.");
        rc = -1;
        goto exit;
    }

    cfg->host = rt_strdup(cJSON_GetObjectItem(root, "host")->valuestring);
    cfg->port = cJSON_GetObjectItem(root, "port")->valueint;
    cfg->client_id = rt_strdup(cJSON_GetObjectItem(root, "client_id")->valuestring);
    cfg->username = rt_strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    cfg->password = rt_strdup(cJSON_GetObjectItem(root, "password")->valuestring);
    cfg->product_key = rt_strdup(cJSON_GetObjectItem(root, "product_key")->valuestring);
    cfg->device_name = rt_strdup(cJSON_GetObjectItem(root, "device_name")->valuestring);
    //属性值上报 /sys/{productKey}/{deviceName}/thing/event/property/post
    snprintf(cfg->params_post_topic, sizeof(cfg->params_post_topic),
             "/sys/%s/%s/thing/event/property/post", cfg->product_key, cfg->device_name);
    //属性值设置 /sys/{productKey}/{deviceName}/thing/service/property/set
    snprintf(cfg->params_set_topic, sizeof(cfg->params_set_topic),
             "/sys/%s/%s/thing/service/property/set", cfg->product_key, cfg->device_name);

    LOG_I("MQTT config read successfully:");
    LOG_I("host:%s", cfg->host);
    LOG_I("port:%d", cfg->port);
    LOG_I("client_id:%s", cfg->client_id);
    LOG_I("username:%s", cfg->username);
    LOG_I("password:%s", cfg->password);
    LOG_I("product_key:%s", cfg->product_key);
    LOG_I("device_name:%s", cfg->device_name);

    rc = 0;

exit:
    if (json_data) rt_free(json_data);
    if (root) cJSON_Delete(root);
    return rc;
}

/*
    {
        "id": "123",
        "version": "1.0",

        "params": {
            "Power": {
            "value": "on",
            "time": 1524448722000
            },
            "WF": {
            "value": 23.6,
            "time": 1524448722000
            }
        },
        "method": "thing.event.property.post"
    }
 */
char *mqtt_create_info(void) {
    static unsigned int id = 0;
    cJSON *root = NULL;
    cJSON *params = NULL;
    char data_str[50];
    char *json_data;

    root = cJSON_CreateObject();
    if (root == NULL) {
        LOG_E("create json root object failed.");
        goto exit;
    }
    params = cJSON_CreateObject();
    if (params == NULL) {
        LOG_E("create json params object failed.");
        goto exit;
    }

    cJSON_AddItemToObject(root, "params", params);

    snprintf(data_str, sizeof(data_str), "%d", id);
    cJSON_AddItemToObject(root, "id", cJSON_CreateString(data_str));
    cJSON_AddItemToObject(root, "method", cJSON_CreateString("thing.event.property.post"));
    cJSON_AddItemToObject(root, "version", cJSON_CreateString("1.0"));

    cJSON_AddItemToObject(params, "temperature", cJSON_CreateNumber(w601.aht10_data.cur_temp));
    cJSON_AddItemToObject(params, "humidity", cJSON_CreateNumber(w601.aht10_data.cur_humi));
    cJSON_AddItemToObject(params, "illumination", cJSON_CreateNumber(w601.ap3216c_data.cur_light));

    cJSON_AddItemToObject(params, "red_led", cJSON_CreateNumber(w601.rgb.red));
    cJSON_AddItemToObject(params, "blue_led", cJSON_CreateNumber(w601.rgb.blue));
    cJSON_AddItemToObject(params, "green_led", cJSON_CreateNumber(w601.rgb.green));

    // snprintf(data_str, sizeof(data_str), "%.2f", temperature);
    // cJSON_AddItemToObject(params, "temperature", cJSON_CreateString(data_str));
    // snprintf(data_str, sizeof(data_str), "%.2f", humidity);
    // cJSON_AddItemToObject(params, "humidity", cJSON_CreateString(data_str));
    // snprintf(data_str, sizeof(data_str), "%.2f", illumination);
    // cJSON_AddItemToObject(params, "illumination", cJSON_CreateString(data_str));

    json_data = cJSON_PrintUnformatted(root);

exit:
    if (root) cJSON_Delete(root);
    return json_data;
}

int mqtt_handle_set(uint8_t *buffer) {
    int rc = -1;
    cJSON *root = NULL;
    cJSON *params = NULL;
    root = cJSON_Parse((const char *)buffer);
    if (root == NULL) {
        LOG_E("json parse failed.");
        rc = -1;
        goto exit;
    }
    params = cJSON_GetObjectItem(root, "params");
    if (params == NULL) {
        LOG_E("cannot find params node.");
        rc = -1;
        goto exit;
    }
    for (cJSON *el = params->child; el != NULL; el = el->next) {
        if (str_compare(el->string, "wol") == 0) {  //一键开机

        } else if (str_compare(el->string, "red_led") == 0) {  //红灯
            led_write(RED, el->valueint ? LED_ON : LED_OFF);
        } else if (str_compare(el->string, "green_led") == 0) {  //绿灯
            led_write(GREEN, el->valueint ? LED_ON : LED_OFF);
        } else if (str_compare(el->string, "blue_led") == 0) {  //蓝灯
            led_write(BLUE, el->valueint ? LED_ON : LED_OFF);
        }
    }
    rc = 0;
exit:
    if (root) cJSON_Delete(root);
    return rc;
}

void mqtt_cfg_free(mqtt_cfg_t *cfg) {
    if (cfg) {
        safe_free(cfg->host);
        safe_free(cfg->client_id);
        safe_free(cfg->password);
        safe_free(cfg->username);
        safe_free(cfg->product_key);
        safe_free(cfg->device_name);
    }
}
