#ifndef __MQTT_MODULE_H
#define __MQTT_MODULE_H

#include <rtthread.h>

#define DEFAULT_CMD_TIMEOUT_MS 30000
#define DEFAULT_CON_TIMEOUT_MS 5000
#define DEFAULT_MQTT_QOS       MQTT_QOS_0
#define DEFAULT_KEEP_ALIVE_SEC 60
#define PRINT_BUFFER_SIZE      80
#define MAX_TOPIC_LENGTH       100

typedef struct _mqtt_cfg {
    char *host;
    uint16_t port;
    char *client_id;
    char *username;
    char *password;
    char *product_key;
    char *device_name;
    char params_post_topic[MAX_TOPIC_LENGTH];
    char params_set_topic[MAX_TOPIC_LENGTH];
} mqtt_cfg_t;

int mqtt_init(void);
#endif /* __MQTT_MODULE_H */
