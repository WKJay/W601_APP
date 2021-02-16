#ifndef __MQTT_JSON_H
#define __MQTT_JSON_H

#include "mqtt_module.h"

#define MQTT_CFG_PATH "webnet/data/mqtt/mqtt_cfg.json"
int mqtt_cfg_parse(mqtt_cfg_t *cfg);
void mqtt_cfg_free(mqtt_cfg_t *cfg);
int mqtt_handle_set(uint8_t *buffer);
char *mqtt_create_info(void);

#endif /* __MQTT_JSON_H */
