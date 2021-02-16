#ifndef __WOLFMQTT_H
#define __WOLFMQTT_H
#include <rtthread.h>
#include "wolfmqtt/mqtt_client.h"

/* MQTT Client state */
typedef enum _MQTTCtxState {
    WMQ_BEGIN = 0,
    WMQ_NET_INIT,
    WMQ_INIT,
    WMQ_TCP_CONN,
    WMQ_MQTT_CONN,
    WMQ_SUB,
    WMQ_PUB,
    WMQ_WAIT_MSG,
    WMQ_UNSUB,
    WMQ_DISCONNECT,
    WMQ_NET_DISCONNECT,
    WMQ_DONE
} MQTTCtxState;

/* MQTT Client context */
/* This is used for the examples as reference */
/* Use of this structure allow non-blocking context */
typedef struct _MQTTCtx {
    MQTTCtxState stat;

    void* app_ctx; /* For storing application specific data */

    /* client and net containers */
    MqttClient client;
    MqttNet net;

    /* temp mqtt containers */
    MqttConnect connect;
    MqttMessage lwt_msg;
    MqttSubscribe subscribe;
    MqttUnsubscribe unsubscribe;
    unsigned int topics_count;
    MqttTopic topics[1];
    MqttPublish publish;
    MqttDisconnect disconnect;

#ifdef WOLFMQTT_SN
    SN_Publish publishSN;
#endif

    /* configuration */
    MqttQoS qos;
    const char* app_name;
    const char* host;
    const char* username;
    const char* password;
    const char* topic_name;
    const char* pub_file;
    const char* client_id;
    byte *tx_buf, *rx_buf;
    int return_code;
    int use_tls;
    int retain;
    int enable_lwt;
#ifdef WOLFMQTT_V5
    int max_packet_size;
#endif
    word32 cmd_timeout_ms;
#if defined(WOLFMQTT_NONBLOCK)
    word32 start_sec; /* used for keep-alive */
#endif
    word16 keep_alive_sec;
    word16 port;
#ifdef WOLFMQTT_V5
    word16 topic_alias;
    word16 topic_alias_max; /* Server property */
#endif
    byte clean_session;
    byte test_mode;
#ifdef WOLFMQTT_V5
    byte subId_not_avail; /* Server property */
    byte enable_eauth;    /* Enhanced authentication */
#endif
    unsigned int dynamicTopic : 1;
    unsigned int dynamicClientId : 1;
#ifdef WOLFMQTT_NONBLOCK
    unsigned int
        useNonBlockMode : 1; /* set to use non-blocking mode.
 network callbacks can return MQTT_CODE_CONTINUE to indicate "would block" */
#endif
} MQTTCtx_t;

MQTTCtx_t* mqtt_ctx_create(const char* host, const char* client_id,
                           const char* username, const char* password,
                           MqttQoS qos, word16 keep_alive_sec,
                           word32 cmd_timeout_ms, MqttMsgCb msg_cb);

int mqtt_ctx_connect(MQTTCtx_t* ctx, int conn_timeout);
int mqtt_subscribe(MQTTCtx_t* ctx, const char* topic_name, MqttQoS qos);
int mqtt_publish(MQTTCtx_t* ctx, uint8_t* data, uint16_t data_len,
                 const char* topic_name);
int mqtt_unsubscribe(MQTTCtx_t* ctx, const char* topic_name);
int mqtt_close(MQTTCtx_t* ctx);
void mqtt_ctx_free(MQTTCtx_t* ctx);
int mqtt_start(MQTTCtx_t* ctx);

#endif /* __WOLFMQTT_H */
