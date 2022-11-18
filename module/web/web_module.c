/*************************************************
Copyright (c) 2019
All rights reserved.
File name:     web_module.c
Description:   网页模块
History:
1. Version:    V1.0.0
Date:      2019-12-08
Author:    WKJay
Modify:
*************************************************/
#include "webnet.h"
#include "aht10_module.h"
#include "smtp_module.h"
#include "board_module.h"
#include "json_module.h"
#include "pin_config.h"
#include "wol_app.h"
#include "web_utils.h"
#include <rtdevice.h>
#include <wn_module.h>
#include <wn_utils.h>
extern const struct webnet_module_upload_entry upload_bin_upload;
extern const struct webnet_module_upload_entry upload_dir_upload;

#define cgi_head()                       \
    ;                                    \
    const char *mimetype;                \
    struct webnet_request *request;      \
    static char *body = NULL;            \
    request = session->request;          \
    mimetype = mime_get_type(".html");   \
    session->request->result_code = 200; \
    webnet_session_set_header(session, mimetype, 200, "Ok", -1);

void cgi_get_aht10_data(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = json_create_aht10_current_data();

    webnet_session_printf(session, body);
    rt_free(body);
}

void cgi_get_info(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = json_create_info();

    webnet_session_printf(session, body);
    rt_free(body);
}

void cgi_get_aht10_saved_data(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = json_create_aht10_saved_data();
    webnet_session_printf(session, body);
    rt_free(body);
}

void cgi_led_toggle(struct webnet_session *session) {
    cgi_head();
    body = body;
    const char *response = "{\"code\":0}";
    const char *id = webnet_request_get_query(request, "id");
    const char *status = webnet_request_get_query(request, "status");

    if (strcmp(id, "red") == 0) {
        if (strcmp(status, "OFF") == 0) {
            led_write(RED, LED_ON);
        } else {
            led_write(RED, LED_OFF);
        }
    } else if (strcmp(id, "green") == 0) {
        if (strcmp(status, "OFF") == 0) {
            led_write(GREEN, LED_ON);
        } else {
            led_write(GREEN, LED_OFF);
        }
    } else if (strcmp(id, "blue") == 0) {
        if (strcmp(status, "OFF") == 0) {
            led_write(BLUE, LED_ON);
        } else {
            led_write(BLUE, LED_OFF);
        }
    }

    webnet_session_printf(session, response);
}

void cgi_get_device_status(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = json_create_device_status();
    webnet_session_printf(session, body);
    rt_free(body);
}

void cgi_smtp_disable(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = body;
    const char *response = "{\"code\":0}";
    smtp_disable();
    webnet_session_printf(session, response);
}

void cgi_smtp_enable(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = body;
    const char *response = "{\"code\":0}";
    smtp_enable();
    webnet_session_printf(session, response);
}

void cgi_smtp_save(struct webnet_session *session) {
    cgi_head();
    body = body;
    const char *response = "{\"code\":0}";
    const char *server_addr = webnet_request_get_query(request, "server_addr");
    const char *server_port = webnet_request_get_query(request, "server_port");
    const char *username = webnet_request_get_query(request, "username");
    const char *password = webnet_request_get_query(request, "password");
    const char *receiver = webnet_request_get_query(request, "receiver");
    const char *temp_warn = webnet_request_get_query(request, "temp_warn");
    smtp_data_config((char *)server_addr, (char *)server_port, (char *)username, (char *)password,
                     (char *)receiver, (char *)temp_warn);
    smtp_enable();
    webnet_session_printf(session, response);
}

void cgi_smtp_get_data(struct webnet_session *session) {
    cgi_head();
    request = request;
    body = json_create_smtp_data();
    webnet_session_printf(session, body);
    rt_free(body);
}

void cgi_wol(struct webnet_session *session) {
    cgi_head();
    char *success = "{\"code\":0}";
    char *error = "{\"code\":-1}";
    const char *value = webnet_request_get_query(request, "value");
    if (wol_process((char *)value) == 0) {
        body = success;
    } else {
        body = error;
    }
    webnet_session_printf(session, body);
}

void cgi_handshake(struct webnet_session *session) {
    cgi_head();
    request = request;  //消除警告
    body = json_create_handshake();
    webnet_session_printf(session, body);
    if (body) rt_free(body);
}

/**
 * Name:    web_module_init
 * Brief:   网页模块初始化
 * Input:   None
 * Return:  Success: 0   Fail: -1
 */
int web_module_init(void) {
    web_file_init();
    webnet_upload_add(&upload_bin_upload);
    webnet_upload_add(&upload_dir_upload);

    webnet_cgi_register("handshake", cgi_handshake);

    webnet_cgi_register("smtp_save", cgi_smtp_save);
    webnet_cgi_register("smtp_get_data", cgi_smtp_get_data);
    webnet_cgi_register("smtp_enable", cgi_smtp_enable);
    webnet_cgi_register("smtp_diasble", cgi_smtp_disable);
    webnet_cgi_register("get_info", cgi_get_info);
    webnet_cgi_register("get_device_status", cgi_get_device_status);
    webnet_cgi_register("get_aht_data", cgi_get_aht10_data);
    webnet_cgi_register("led_toggle", cgi_led_toggle);
    webnet_cgi_register("get_aht_saved_data", cgi_get_aht10_saved_data);
    webnet_cgi_register("mystery", cgi_wol);
    return webnet_init();
}
