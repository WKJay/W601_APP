/*************************************************
Copyright (c) 2019
All rights reserved.
File name:     board_module.c
Description:
History:
1. Version:    V1.0.0
Date:      2019-12-09
Author:    WKJay
Modify:
*************************************************/

#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "board_module.h"
#include "W601_app.h"
#include "cJSON.h"

void led_module_init(void) {
    rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LED_G, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LED_B, PIN_MODE_OUTPUT);
    w601.rgb.blue = 0;
    w601.rgb.green = 0;
    w601.rgb.red = 0;
}

void led_write(led_id_e led, uint8_t status) {
    switch (led) {
        case RED:
            rt_pin_write(PIN_LED_R, status);
            w601.rgb.red = ((status == LED_ON) ? 1 : 0);
            break;
        case GREEN:
            rt_pin_write(PIN_LED_G, status);
            w601.rgb.green = ((status == LED_ON) ? 1 : 0);
            break;
        case BLUE:
            rt_pin_write(PIN_LED_B, status);
            w601.rgb.blue = ((status == LED_ON) ? 1 : 0);
            break;
        default:
            break;
    }
}

char *json_create_device_status(void) {
    char *json_data = RT_NULL;
    cJSON *root = cJSON_CreateObject();

    if (w601.rgb.red == LED_ON) {
        cJSON_AddItemToObject(root, "redLedStatus", cJSON_CreateString("ON"));
    } else {
        cJSON_AddItemToObject(root, "redLedStatus", cJSON_CreateString("OFF"));
    }

    if (w601.rgb.green == LED_ON) {
        cJSON_AddItemToObject(root, "greenLedStatus", cJSON_CreateString("ON"));
    } else {
        cJSON_AddItemToObject(root, "greenLedStatus", cJSON_CreateString("OFF"));
    }

    if (w601.rgb.blue == LED_ON) {
        cJSON_AddItemToObject(root, "blueLedStatus", cJSON_CreateString("ON"));
    } else {
        cJSON_AddItemToObject(root, "blueLedStatus", cJSON_CreateString("OFF"));
    }
    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}
