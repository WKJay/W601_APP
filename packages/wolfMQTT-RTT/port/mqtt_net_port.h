#ifndef __MQTT_NET_PORT_H
#define __MQTT_NET_PORT_H

#include "wolfmqtt.h"

#ifdef __cplusplus
    extern "C" {
#endif

/* Default MQTT host broker to use, when none is specified in the examples */
#ifndef DEFAULT_MQTT_HOST
#define DEFAULT_MQTT_HOST       "test.mosquitto.org" /* broker.hivemq.com */
#endif

/* Functions used to handle the MqttNet structure creation / destruction */
int MqttClientNet_Init(MqttNet* net, MQTTCtx_t* mqttCtx);
int MqttClientNet_DeInit(MqttNet* net);
int MqttClientNet_Wake(MqttNet* net);

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* __MQTT_NET_PORT_H */
