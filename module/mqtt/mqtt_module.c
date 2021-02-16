/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     mqtt.c
 Description:
 History:
 1. Version:
    Date:       2021-02-14
    Author:     WKJay
    Modify:
*************************************************/
#include <rtthread.h>
#include <wolfmqtt.h>
#include "mqtt_json.h"
#include "W601_app.h"

#define LOG_TAG "mqtt"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

MQTTCtx_t* mqtt_ctx;
mqtt_cfg_t mqtt_cfg;
mqtt_cfg_t cfg;


int mqtt_publish_info(void) {
    int rc = -1;
    char* json_data = NULL;

    json_data = mqtt_create_info();
    if (json_data == NULL) {
        return -1;
    }

    // printf("%s",json_data);
    rc = mqtt_publish(mqtt_ctx, (uint8_t*)json_data, strlen(json_data), mqtt_cfg.params_post_topic);
    rt_free(json_data);
    return rc;
}

static int mqtt_message_cb(MqttClient* client, MqttMessage* msg, byte msg_new, byte msg_done) {
    MQTTCtx_t* mqttCtx = (MQTTCtx_t*)client->ctx;
    (void)mqttCtx;

    if (msg_new) {
        // dosomething
    }

    if (msg_done) {
        if (rt_memcmp(msg->topic_name, mqtt_cfg.params_set_topic, msg->topic_name_len) == 0) {
            if(mqtt_handle_set(msg->buffer) == 0){
                 mqtt_publish_info();
            }
        }
    }

    /* Return negative to terminate publish processing */
    return MQTT_CODE_SUCCESS;
}

void mqtt_thread_entry(void* param) {
    while (1) {
        mqtt_publish_info();
        rt_thread_mdelay(60000);
    }
}

int mqtt_init(void) {
    rt_thread_t tid = NULL;
    rt_memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));
    if (mqtt_cfg_parse(&mqtt_cfg) < 0) {
        return -1;
    }
    mqtt_ctx = mqtt_ctx_create(mqtt_cfg.host, mqtt_cfg.client_id, mqtt_cfg.username,
                               mqtt_cfg.password, DEFAULT_MQTT_QOS, DEFAULT_KEEP_ALIVE_SEC,
                               DEFAULT_CMD_TIMEOUT_MS, mqtt_message_cb);
    if (mqtt_ctx == NULL) {
        printf("create mqtt context failed.\r\n");
        return -1;
    }
    if (mqtt_ctx_connect(mqtt_ctx, DEFAULT_CON_TIMEOUT_MS) < 0) {
        mqtt_ctx_free(mqtt_ctx);
        return -1;
    }
    if (mqtt_subscribe(mqtt_ctx, mqtt_cfg.params_set_topic, DEFAULT_MQTT_QOS) < 0) {
        mqtt_close(mqtt_ctx);
        return -1;
    }

    mqtt_start(mqtt_ctx);

    tid = rt_thread_create("mqtt_app", mqtt_thread_entry, NULL, 2048, 5, 5);
    if (tid) {
        rt_thread_startup(tid);
    } else {
        LOG_E("create mqtt app thread failed");
        return -1;
    }

    return 0;
}

int mqtt_publish_test(void) {
    const char* TEST_STR = "this is a test";
    int str_len = strlen(TEST_STR);
    mqtt_publish(mqtt_ctx, (uint8_t*)TEST_STR, str_len, "/wolfMQTT/test/");
    return 0;
}
int mqtt_cfg_parse_test(void) {
    mqtt_cfg_t mqtt_cfg;
    rt_memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));
    mqtt_cfg_parse(&mqtt_cfg);
    mqtt_cfg_free(&mqtt_cfg);
    return 0;
}
MSH_CMD_EXPORT(mqtt_publish_test, mqtt_publish_test);
MSH_CMD_EXPORT(mqtt_cfg_parse_test, mqtt_cfg_parse_test);
