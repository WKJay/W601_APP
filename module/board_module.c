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
#include "cJSON.h"

board_device_t w601_board;

void led_module_init(void)
{
    rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LED_G, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LED_B, PIN_MODE_OUTPUT);
    w601_board.blue_led_status = LED_OFF;
    w601_board.green_led_status = LED_OFF;
    w601_board.red_led_status = LED_OFF;
}

void led_write(led_id_e led, uint8_t status)
{
    switch (led)
    {
    case RED:
        rt_pin_write(PIN_LED_R, status);
        w601_board.red_led_status = status;
        break;
    case GREEN:
        rt_pin_write(PIN_LED_G, status);
        w601_board.green_led_status = status;
        break;
    case BLUE:
        rt_pin_write(PIN_LED_B, status);
        w601_board.blue_led_status = status;
        break;
    default:
        break;
    }
}

char *json_create_device_status(void)
{
    char *json_data = RT_NULL;
    cJSON *root = cJSON_CreateObject();

    if (w601_board.red_led_status == LED_ON)
    {
        cJSON_AddItemToObject(root, "redLedStatus", cJSON_CreateString("ON"));
    }
    else
    {
        cJSON_AddItemToObject(root, "redLedStatus", cJSON_CreateString("OFF"));
    }

    if (w601_board.green_led_status == LED_ON)
    {
        cJSON_AddItemToObject(root, "greenLedStatus", cJSON_CreateString("ON"));
    }
    else
    {
        cJSON_AddItemToObject(root, "greenLedStatus", cJSON_CreateString("OFF"));
    }

    if (w601_board.blue_led_status == LED_ON)
    {
        cJSON_AddItemToObject(root, "blueLedStatus", cJSON_CreateString("ON"));
    }
    else
    {
        cJSON_AddItemToObject(root, "blueLedStatus", cJSON_CreateString("OFF"));
    }
    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}
