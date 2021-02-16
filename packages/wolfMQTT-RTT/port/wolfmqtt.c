/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     wolfmqtt.c
 Description:
 History:
 1. Version:
    Date:       2021-02-04
    Author:     WKJay
    Modify:
*************************************************/
#include "wolfmqtt.h"
#include "wolfmqtt_cfg.h"
#include "mqtt_net_port.h"

word16 mqtt_get_packetid(void) {
    static int mPacketIdLast = 0;
    /* Check rollover */
    if (mPacketIdLast >= MAX_PACKET_ID) {
        mPacketIdLast = 0;
    }

    return ++mPacketIdLast;
}

void _free_ctx(MQTTCtx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (ctx->dynamicTopic && ctx->topic_name) {
        WOLFMQTT_FREE((char *)ctx->topic_name);
        ctx->topic_name = NULL;
    }
    if (ctx->dynamicClientId && ctx->client_id) {
        WOLFMQTT_FREE((char *)ctx->client_id);
        ctx->client_id = NULL;
    }

    WOLFMQTT_FREE(ctx);
}

void mqtt_ctx_free(MQTTCtx_t *ctx) {
    if (ctx->tx_buf) WOLFMQTT_FREE(ctx->tx_buf);
    if (ctx->rx_buf) WOLFMQTT_FREE(ctx->rx_buf);

    /* Cleanup network */
    MqttClientNet_DeInit(&ctx->net);

    MqttClient_DeInit(&ctx->client);

    _free_ctx(ctx);
}

/* Create mqtt context */
MQTTCtx_t *mqtt_ctx_create(const char *host, const char *client_id, const char *username,
                           const char *password, MqttQoS qos, word16 keep_alive_sec,
                           word32 cmd_timeout_ms, MqttMsgCb msg_cb) {
    int rc = -1;
    MQTTCtx_t *ctx = rt_malloc(sizeof(MQTTCtx_t));
    if (ctx == NULL) {
        PRINTF("cannot create mqtt context.");
        return NULL;
    }
    rt_memset(ctx, 0, sizeof(*ctx));

    ctx->host = host;
    ctx->qos = qos;
    ctx->clean_session = 1;
    ctx->keep_alive_sec = keep_alive_sec;
    ctx->client_id = client_id;
    ctx->username = username;
    ctx->password = password;
    ctx->cmd_timeout_ms = cmd_timeout_ms;

    rc = MqttClientNet_Init(&ctx->net, ctx);

    /* setup tx/rx buffers */
    ctx->tx_buf = (byte *)rt_malloc(MAX_BUFFER_SIZE);
    ctx->rx_buf = (byte *)rt_malloc(MAX_BUFFER_SIZE);
    if ((ctx->tx_buf && ctx->rx_buf) == NULL) {
        PRINTF("cannot allocate tx or rx buffer.");
        goto exit;
    }

    /* Initialize MqttClient structure */
    rc = MqttClient_Init(&ctx->client, &ctx->net, msg_cb, ctx->tx_buf, MAX_BUFFER_SIZE, ctx->rx_buf,
                         MAX_BUFFER_SIZE, ctx->cmd_timeout_ms);
    PRINTF("MQTT Init: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        goto exit;
    }

    /* The client.ctx will be stored in the cert callback ctx during
       MqttSocket_Connect for use by mqtt_tls_verify_cb */
    ctx->client.ctx = ctx;

    return ctx;

exit:
    mqtt_ctx_free(ctx);
    return NULL;
}

/* Connect to broker */
int mqtt_ctx_connect(MQTTCtx_t *ctx, int conn_timeout) {
    int rc =
        MqttClient_NetConnect(&ctx->client, ctx->host, ctx->port, conn_timeout, ctx->use_tls, NULL);
    PRINTF("MQTT Socket Connect: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        return -1;
    }

    /* Build connect packet */
    XMEMSET(&ctx->connect, 0, sizeof(MqttConnect));
    ctx->connect.keep_alive_sec = ctx->keep_alive_sec;
    ctx->connect.clean_session = ctx->clean_session;
    ctx->connect.client_id = ctx->client_id;

    /* Last will and testament sent by broker to subscribers
        of topic when broker connection is lost */
    XMEMSET(&ctx->lwt_msg, 0, sizeof(ctx->lwt_msg));
    ctx->connect.lwt_msg = &ctx->lwt_msg;
    ctx->connect.enable_lwt = ctx->enable_lwt;
    if (ctx->enable_lwt) {
        /* Send client id in LWT payload */
        ctx->lwt_msg.qos = ctx->qos;
        ctx->lwt_msg.retain = 0;
        ctx->lwt_msg.topic_name = "wolfMQTT/test/lwttopic";
        ctx->lwt_msg.buffer = (byte *)ctx->client_id;
        ctx->lwt_msg.total_len = (word16)XSTRLEN(ctx->client_id);
    }
    /* Optional authentication */
    ctx->connect.username = ctx->username;
    ctx->connect.password = ctx->password;

    /* Send Connect and wait for Connect Ack */
    rc = MqttClient_Connect(&ctx->client, &ctx->connect);
    PRINTF("MQTT Connect: Proto (%s), %s (%d)", MqttClient_GetProtocolVersionString(&ctx->client),
           MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        return -1;
    }

    /* Validate Connect Ack info */
    PRINTF("MQTT Connect Ack: Return Code %u, Session Present %d", ctx->connect.ack.return_code,
           (ctx->connect.ack.flags & MQTT_CONNECT_ACK_FLAG_SESSION_PRESENT) ? 1 : 0);
    return rc;
}

int mqtt_subscribe(MQTTCtx_t *ctx, const char *topic_name, MqttQoS qos) {
    int rc = -1;
    if (ctx->topics_count < 10) {
        ctx->topics_count++;
    } else {
        PRINTF("already subscribed %d topics, no more room for another topic.", ctx->topics_count);
        return -1;
    }
    ctx->topics[ctx->topics_count - 1].topic_filter = topic_name;
    ctx->topics[ctx->topics_count - 1].qos = qos;

    ctx->subscribe.packet_id = mqtt_get_packetid();
    ctx->subscribe.topic_count = 1;
    ctx->subscribe.topics = &ctx->topics[ctx->topics_count - 1];

    rc = MqttClient_Subscribe(&ctx->client, &ctx->subscribe);

    PRINTF("MQTT Subscribe: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    return rc;
}

int mqtt_publish(MQTTCtx_t *ctx, uint8_t *data, uint16_t data_len, const char *topic_name) {
    int rc = -1;
    XMEMSET(&ctx->publish, 0, sizeof(MqttPublish));
    ctx->publish.qos = ctx->qos;
    ctx->publish.retain = 0;
    ctx->publish.duplicate = 0;
    ctx->publish.packet_id = mqtt_get_packetid();

    ctx->publish.topic_name = topic_name;
    ctx->publish.buffer = data;
    ctx->publish.total_len = data_len;

    rc = MqttClient_Publish(&ctx->client, &ctx->publish);
    // PRINTF("MQTT Publish: Topic %s, %s (%d)", topic_name,
    //        MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        PRINTF("MQTT Publish: Topic %s, %s (%d)", topic_name, MqttClient_ReturnCodeToString(rc),
               rc);
        return -1;
    }
    return rc;
}

int mqtt_unsubscribe(MQTTCtx_t *ctx, const char *topic_name) {
    int rc = -1;
    XMEMSET(&ctx->unsubscribe, 0, sizeof(MqttUnsubscribe));
    ctx->unsubscribe.packet_id = mqtt_get_packetid();
    ctx->unsubscribe.topic_count = 1;
    for (int i = 0; i < ctx->topics_count; i++) {
        if (strcmp(ctx->topics[i].topic_filter, topic_name) == 0) {
            ctx->unsubscribe.topics = &ctx->topics[i];
            break;
        }
        if (i == ctx->topics_count - 1) {
            PRINTF("unsubscribe error,cannot find topic %s", topic_name);
            return -1;
        }
    }
    rc = MqttClient_Unsubscribe(&ctx->client, &ctx->unsubscribe);
    PRINTF("MQTT Unsubscribe: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        return -1;
    }
		return 0;
}

int mqtt_unsubscribe_all(MQTTCtx_t *ctx) {
    int rc = -1;
    XMEMSET(&ctx->unsubscribe, 0, sizeof(MqttUnsubscribe));
    ctx->unsubscribe.packet_id = mqtt_get_packetid();
    ctx->unsubscribe.topic_count = ctx->topics_count;
    ctx->unsubscribe.topics = ctx->topics;

    rc = MqttClient_Unsubscribe(&ctx->client, &ctx->unsubscribe);

    PRINTF("MQTT Unsubscribe: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        return -1;
    }
    return 0;
}

int mqtt_close(MQTTCtx_t *ctx) {
    int rc = -1;
    rc = MqttClient_Disconnect_ex(&ctx->client, &ctx->disconnect);
    PRINTF("MQTT Disconnect: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        return -1;
    }
    rc = MqttClient_NetDisconnect(&ctx->client);
    PRINTF("MQTT Socket Disconnect: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);

    mqtt_ctx_free(ctx);

    return rc;
}

void matt_thread(void *param) {
    MQTTCtx_t *ctx = (MQTTCtx_t *)param;
    int rc = -1;
    while (1) {
        rc = MqttClient_WaitMessage(&ctx->client, ctx->cmd_timeout_ms);
        if (rc == MQTT_CODE_ERROR_TIMEOUT) {
            /* Keep Alive */
            rc = MqttClient_Ping(&ctx->client);
            if (rc != MQTT_CODE_SUCCESS) {
                PRINTF("MQTT Ping Keep Alive Error: %s (%d)", MqttClient_ReturnCodeToString(rc),
                       rc);
                break;
            }
        } else if (rc != MQTT_CODE_SUCCESS) {
            /* There was an error */
            PRINTF("MQTT Message Wait: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
            break;
        }
    }
    mqtt_unsubscribe_all(ctx);
    mqtt_close(ctx);
}

int mqtt_start(MQTTCtx_t *ctx) {
    rt_thread_t tid = rt_thread_create("mqtt", matt_thread, ctx, MQTT_THREAD_STACK, 5, 5);
    if (tid) {
        rt_thread_startup(tid);
        return 0;
    } else {
        rt_kprintf("cannot crete mqtt thread.\r\n");
        return -1;
    }
}
