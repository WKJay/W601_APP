/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     ulog_tcp.c
 Description:
 History:
 1. Version:
    Date:       2020-12-08
    Author:     wangjunjie
    Modify:
*************************************************/
#include <ulog.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rthw.h>

#ifdef SAL_USING_POSIX
#include <sys/select.h>
#else
#include <lwip/select.h>
#endif

#include "ulog_cfg.h"

#define MAKEU32(a, b, c, d)                                        \
    (((uint32_t)((a)&0xff) << 24) | ((uint32_t)((b)&0xff) << 16) | \
     ((uint32_t)((c)&0xff) << 8) | (uint32_t)((d)&0xff))

#define TRUE  1
#define FALSE 0

/* log manager struct */
typedef struct _ulog_tcp {
    int socket;
    uint8_t ip[4];
    uint16_t port;
    rt_tick_t timeout;
    struct _ulog_tcp *next;
} ulog_tcp_t;

static ulog_tcp_t *ulog_tcp_list = NULL;
static struct ulog_backend ulog_tcp_backend;
static uint8_t ulog_tcp_shutdown = 0;
static rt_mutex_t ulog_tcp_mutex;

static int ulog_tcp_close_one_connection(ulog_tcp_t *conn, uint8_t del) {
    ulog_tcp_t *iter;

    if (del) {
        if (ulog_tcp_list == conn)
            ulog_tcp_list = conn->next;
        else {
            for (iter = ulog_tcp_list; iter; iter = iter->next) {
                if (iter->next == conn) {
                    iter->next = conn->next;
                    break;
                }
            }
        }
    }

    if (conn->socket > 0) {
        closesocket(conn->socket);
        conn->socket = -1;
    }

    if (del) {
        rt_free(conn);
    } else {
        conn->timeout = rt_tick_get() +
                        rt_tick_from_millisecond(ULOG_TCP_CONN_RETRY_TIMEOUT);
    }

    return 0;
}
static void ulog_tcp_close_all_connection(uint8_t del) {
    ulog_tcp_t *cur_conn, *next_conn;
    for (cur_conn = ulog_tcp_list; cur_conn; cur_conn = next_conn) {
        next_conn = cur_conn->next;
        ulog_tcp_close_one_connection(cur_conn, del);
    }
}

static int ulog_tcp_list_set_fds(fd_set *readset) {
    int maxfd = 0;
    ulog_tcp_t *iter;

    for (iter = ulog_tcp_list; iter; iter = iter->next) {
        if (iter->socket <= 0) continue;
        if (maxfd < iter->socket + 1) maxfd = iter->socket + 1;
        FD_SET(iter->socket, readset);
    }

    return maxfd;
}
static void ulog_tcp_list_handle_fds(fd_set *readset) {
    char data;
    ulog_tcp_t *utcp = NULL, *next = NULL;

    for (utcp = ulog_tcp_list; utcp; utcp = next) {
        next = utcp->next;
        if (utcp->socket < 0) continue;

        if (FD_ISSET(utcp->socket, readset)) {
            while (1) {
                int ret = recv(utcp->socket, &data, 1, 0);
                if (ret == 0) {
                    rt_kprintf(
                        "ulog tcp connection %d been closed by peer.\r\n",
                        utcp->socket);
                    ulog_tcp_close_one_connection(utcp, FALSE);
                    break;
                } else if (ret < 0) {
                    if (errno != EWOULDBLOCK) {
                        // rt_kprintf("ulog tcp connection %d error,now
                        // close\r\n",
                        //            utcp->socket);
                        ulog_tcp_close_one_connection(utcp, FALSE);
                        break;
                    } else {
                        // continue
                        break;
                    }
                }
            }
        }
    }
}

static void _ulog_tcp_output(struct ulog_backend *backend, rt_uint32_t level,
                             const char *tag, rt_bool_t is_raw, const char *log,
                             size_t len) {
    ulog_tcp_t *cur = ulog_tcp_list;
    for (; cur; cur = cur->next) {
        if (cur->socket > 0) {
            send(cur->socket, log, len, 0);
        }
    }
}

static void _ulog_tcp_deinit(struct ulog_backend *backend) {
    ulog_tcp_shutdown = 1;
}

static int ulog_tcp_connect(ulog_tcp_t *utcp) {
    struct sockaddr_in server_addr;
    unsigned long ul = 1;

    if (utcp->socket <= 0) {
        utcp->socket = socket(AF_INET, SOCK_STREAM, 0);
        if (utcp->socket <= 0) goto err;
    }
    rt_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =
        PP_HTONL(MAKEU32(utcp->ip[0], utcp->ip[1], utcp->ip[2], utcp->ip[3]));
    server_addr.sin_port = htons(utcp->port);

    if (ioctlsocket(utcp->socket, FIONBIO, (unsigned long *)&ul) < 0) {
        rt_kprintf("ulog tcp set non-block failed.\r\n");
        goto err;
    }

    // rt_kprintf("ulog tcp connect %d\r\n", utcp->socket);

    if (connect(utcp->socket, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) < 0) {
        if (errno != EINPROGRESS) {
            rt_kprintf("ulog tcp connect failed.\r\n");
            goto err;
        }
    }

    return 0;
err:
    ulog_tcp_close_one_connection(utcp, FALSE);
    utcp->timeout =
        rt_tick_get() + rt_tick_from_millisecond(ULOG_TCP_CONN_RETRY_TIMEOUT);
    return -1;
}

static void ulog_tcp_handle_reconnect(void) {
    ulog_tcp_t *iter;

    for (iter = ulog_tcp_list; iter; iter = iter->next) {
        if (iter->socket <= 0) {
            if ((rt_tick_get() - iter->timeout) < (RT_TICK_MAX / 2)) {
                ulog_tcp_connect(iter);
            }
        }
    }
}

void ulog_tcp_thread(void *param) {
    fd_set readset;
    int sockfd, maxfd;

    struct timeval timeout;
    /* set ulog tcp select timeout */
    timeout.tv_sec = ULOG_TCP_SELECT_TIMEOUT / 1000;
    timeout.tv_usec = (ULOG_TCP_SELECT_TIMEOUT % 1000) * 1000;
    ulog_tcp_shutdown = 0;

    for (;;) {
        if (ulog_tcp_shutdown) {
            goto shutdown;
        }
        FD_ZERO(&readset);

        rt_mutex_take(ulog_tcp_mutex, RT_WAITING_FOREVER);
        maxfd = ulog_tcp_list_set_fds(&readset);

        sockfd = select(maxfd, &readset, NULL, NULL, &timeout);
        ulog_tcp_handle_reconnect();
        if (sockfd == 0) {
            rt_mutex_release(ulog_tcp_mutex);
            continue;
        } else if (sockfd < 0) {
            ulog_tcp_close_all_connection(FALSE);
            rt_thread_mdelay(1000);
        }

        ulog_tcp_list_handle_fds(&readset);
        rt_mutex_release(ulog_tcp_mutex);
    }
shutdown:
    if (ulog_tcp_list) {
        ulog_tcp_close_all_connection(TRUE);
    }
    if (ulog_tcp_mutex) {
        rt_mutex_take(ulog_tcp_mutex, RT_WAITING_FOREVER);
        rt_mutex_release(ulog_tcp_mutex);
        ulog_tcp_mutex = NULL;
    }
    // exit thread
    return;
}

void _ulog_tcp_init(struct ulog_backend *backend) {
    rt_thread_t tid = NULL;
    if (ulog_tcp_mutex == NULL) {
        ulog_tcp_mutex = rt_mutex_create("utcp", RT_IPC_FLAG_FIFO);
        if (ulog_tcp_mutex == NULL) {
            rt_kprintf("ulog tcp mutex create failed.\r\n");
            return;
        }
    }
    tid = rt_thread_create("ulog_tcp", ulog_tcp_thread, NULL, 2048, 10, 5);
    if (tid) {
        if (rt_thread_startup(tid) == RT_EOK) {
            return;
        } else {
            rt_kprintf("ulog tcp thread start failed.\r\n");
            return;
        }
    } else {
        rt_kprintf("ulog tcp thread create failed.\r\n");
        return;
    }
    // do nothing
}

int ulog_tcp_add_server(uint8_t *ip, uint16_t port) {
    int server_cnt = 0, ret;
    ulog_tcp_t *cur;

    if (ulog_tcp_shutdown == 1) {
        rt_kprintf("ulog tcp backend has been shut down.\r\n");
        return -1;
    }

    if (rt_mutex_take(ulog_tcp_mutex, ULOG_TCP_MUTEX_TIMEOUT) != RT_EOK) {
        rt_kprintf("take ulog tcp mutex failed.\r\n");

        return -1;
    }

    for (cur = ulog_tcp_list; cur; cur = cur->next) {
        server_cnt++;
        if (rt_memcmp(cur->ip, ip, 4) == 0 && cur->port == port) {
            rt_kprintf("ulog tcp server %d.%d.%d.%d:%d already added.", ip[0],
                       ip[1], ip[2], ip[3], port);
            ret = -1;
            goto end;
        }
    }
    if (server_cnt > ULOG_TCP_MAX_SERVER_COUNT) {
        rt_kprintf("no enough tcp server space.\r\n");
        ret = -1;
        goto end;
    }
    cur = (ulog_tcp_t *)rt_malloc((sizeof(ulog_tcp_t)));
    if (cur == NULL) {
        rt_kprintf("cannot allocate ulog_tcp_t memory.\r\n");
        ret = -1;
        goto end;
    }
    rt_memset(cur, 0, sizeof(ulog_tcp_t));
    rt_memcpy(cur->ip, ip, 4);
    cur->port = port;

    if (ulog_tcp_list) {
        /* add server to list */
        cur->next = ulog_tcp_list;
        ulog_tcp_list = cur;
    } else {
        ulog_tcp_list = cur;
    }
    /* do connection */
    ulog_tcp_connect(cur);

    ret = 0;
    goto end;
end:

    rt_mutex_release(ulog_tcp_mutex);

    return ret;
}

int ulog_tcp_delete_server(uint8_t *ip, uint16_t port) {
    ulog_tcp_t *iter = NULL;
    int ret;
    if (ulog_tcp_shutdown == 1) {
        rt_kprintf("ulog tcp backend has been shut down.\r\n");
        return -1;
    }

    if (rt_mutex_take(ulog_tcp_mutex, ULOG_TCP_MUTEX_TIMEOUT) != RT_EOK) {
        rt_kprintf("take ulog tcp mutex failed.\r\n");
        return -1;
    }

    for (iter = ulog_tcp_list; iter; iter = iter->next) {
        if ((rt_memcmp(iter->ip, ip, 4) == 0) && (iter->port == port)) {
            ulog_tcp_close_one_connection(iter, TRUE);
            ret = 0;
            goto end;
        }
    }
    /* not found */
    ret = -1;
end:
    rt_mutex_release(ulog_tcp_mutex);

    return ret;
}

int ulog_tcp_init(void) {
    ulog_init();
    ulog_tcp_backend.output = _ulog_tcp_output;
    ulog_tcp_backend.init = _ulog_tcp_init;
    ulog_tcp_backend.deinit = _ulog_tcp_deinit;
    ulog_backend_register(&ulog_tcp_backend, "ulog_tcp", RT_TRUE);
    return 0;
}
INIT_PREV_EXPORT(ulog_tcp_init);

/* MSH CMD */
void ulog_tcp_testlog(void) {
    LOG_E("TEST_E");
    LOG_I("TEST_I");
    LOG_D("TEST_D");
    LOG_W("TEST_W");
}
MSH_CMD_EXPORT(ulog_tcp_testlog, test log function);

static void print_server_list(void) {
    ulog_tcp_t *iter = NULL;
    int index = 0;
    rt_mutex_take(ulog_tcp_mutex, ULOG_TCP_MUTEX_TIMEOUT);

    if (ulog_tcp_list == NULL) {
        rt_kprintf("no active log server connected.\r\n");
    } else {
        rt_kprintf("index ip:port\r\n");
        for (iter = ulog_tcp_list; iter; iter = iter->next) {
            index++;
            rt_kprintf("%5d %d.%d.%d.%d:%d\r\n", index, iter->ip[0],
                       iter->ip[1], iter->ip[2], iter->ip[3], iter->port);
        }
    }
    rt_mutex_release(ulog_tcp_mutex);
}
void ulog_tcp(int argc, char **argv) {
    int ip[4], port;
    uint8_t ip_u8[4];

    if (argc == 2) {
        if (argv[1][0] == 'l') {
            print_server_list();
            return;
        } else {
            goto err_param;
        }
    } else if (argc == 3) {
        if (argv[1][0] != 'a' && argv[1][0] != 'd') goto err_param;
        if (sscanf(argv[2], "%d.%d.%d.%d:%d", &ip[0], &ip[1], &ip[2], &ip[3],
                   &port) != 5)
            goto err_param;

        if ((ip[0] <= 0 || ip[0] > 255) || (ip[1] < 0 || ip[1] > 255) ||
            (ip[2] < 0 || ip[2] > 255) || (ip[3] < 0 || ip[3] > 255) ||
            (port < 0 || port > 65535))
            goto err_param;

        ip_u8[0] = ip[0];
        ip_u8[1] = ip[1];
        ip_u8[2] = ip[2];
        ip_u8[3] = ip[3];

        if (argv[1][0] == 'a')
            ulog_tcp_add_server(ip_u8, port);
        else if (argv[1][0] == 'd')
            ulog_tcp_delete_server(ip_u8, port);

        return;
    } else {
        goto err_param;
    }

err_param:
    rt_kprintf("bad parameter! input: ulog_tcp <a>/<d> <ip:port>\r\n");
}
MSH_CMD_EXPORT(ulog_tcp, ulog tcp cmd);
