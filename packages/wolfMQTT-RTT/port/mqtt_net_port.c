/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     mqtt.c
 Description:
 History:
 1. Version:
    Date:       2021-02-02
    Author:     WKJay
    Modify:
*************************************************/
#include "wolfmqtt/mqtt_client.h"
#include "wolfmqtt.h"

#include <rtthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdev.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <dfs_posix.h>

/* Setup defaults */
#ifndef SOCK_OPEN
#define SOCK_OPEN socket
#endif
#ifndef SOCKET_T
#define SOCKET_T int
#endif
#ifndef SOERROR_T
#define SOERROR_T int
#endif
#ifndef SELECT_FD
#define SELECT_FD(fd) ((fd) + 1)
#endif
#ifndef SOCKET_INVALID
#define SOCKET_INVALID ((SOCKET_T)0)
#endif
#ifndef SOCK_CONNECT
#define SOCK_CONNECT connect
#endif
#ifndef SOCK_SEND
#define SOCK_SEND(s, b, l, f) send((s), (b), (size_t)(l), (f))
#endif
#ifndef SOCK_RECV
#define SOCK_RECV(s, b, l, f) recv((s), (b), (size_t)(l), (f))
#endif
#ifndef SOCK_CLOSE
#define SOCK_CLOSE close
#endif
#ifndef SOCK_ADDR_IN
#define SOCK_ADDR_IN struct sockaddr_in
#endif
#ifdef SOCK_ADDRINFO
#define SOCK_ADDRINFO struct addrinfo
#endif

/* Local context for Net callbacks */
typedef enum {
    SOCK_BEGIN = 0,
    SOCK_CONN,
} NB_Stat;

typedef struct _SocketContext {
    SOCKET_T fd;
    NB_Stat stat;
    SOCK_ADDR_IN addr;
#ifdef MICROCHIP_MPLAB_HARMONY
    word32 bytes;
#endif
#if defined(WOLFMQTT_MULTITHREAD) && defined(WOLFMQTT_ENABLE_STDIN_CAP)
    /* "self pipe" -> signal wake sleep() */
    SOCKET_T pfd[2];
#endif
    MQTTCtx_t* mqttCtx;
} SocketContext;

#ifdef RT_USING_SAL

#ifndef WOLFMQTT_NO_TIMEOUT
static void setup_timeout(struct timeval* tv, int timeout_ms) {
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms % 1000) * 1000;

    /* Make sure there is a minimum value specified */
    if (tv->tv_sec < 0 || (tv->tv_sec == 0 && tv->tv_usec <= 0)) {
        tv->tv_sec = 0;
        tv->tv_usec = 100;
    }
}

#ifdef WOLFMQTT_NONBLOCK
static void tcp_set_nonblocking(SOCKET_T* sockfd) {
    int flags = fcntl(*sockfd, F_GETFL, 0);
    if (flags < 0) PRINTF("fcntl get failed!");
    flags = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0) PRINTF("fcntl set failed!");
}
#endif /* WOLFMQTT_NONBLOCK */
#endif /* !WOLFMQTT_NO_TIMEOUT */

static int NetConnect(void* context, const char* host, word16 port,
                      int timeout_ms) {
    SocketContext* sock = (SocketContext*)context;
    int type = SOCK_STREAM;
    int rc = -1;
    SOERROR_T so_error = 0;
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    MQTTCtx_t* mqttCtx = sock->mqttCtx;

    /* Get address information for host and locate IPv4 */
    switch (sock->stat) {
        case SOCK_BEGIN: {
            PRINTF("NetConnect: Host %s, Port %u, Timeout %d ms, Use TLS %d",
                   host, port, timeout_ms, mqttCtx->use_tls);

            XMEMSET(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            XMEMSET(&sock->addr, 0, sizeof(sock->addr));
            sock->addr.sin_family = AF_INET;

            rc = getaddrinfo(host, NULL, &hints, &result);
            if (rc == 0) {
                struct addrinfo* result_i = result;

                if (!result) {
                    rc = -1;
                    goto exit;
                }

                /* prefer ip4 addresses */
                while (result_i) {
                    if (result_i->ai_family == AF_INET) break;
                    result_i = result_i->ai_next;
                }

                if (result_i) {
                    sock->addr.sin_port = htons(port);
                    sock->addr.sin_family = AF_INET;
                    sock->addr.sin_addr =
                        ((SOCK_ADDR_IN*)(result_i->ai_addr))->sin_addr;
                } else {
                    rc = -1;
                }

                freeaddrinfo(result);
            }
            if (rc != 0) goto exit;

            /* Default to error */
            rc = -1;

            /* Create socket */
            sock->fd = SOCK_OPEN(sock->addr.sin_family, type, 0);
            if (sock->fd == SOCKET_INVALID) goto exit;

            sock->stat = SOCK_CONN;

            FALL_THROUGH;
        }

        case SOCK_CONN: {
#ifndef WOLFMQTT_NO_TIMEOUT
            fd_set fdset;
            struct timeval tv;

            /* Setup timeout and FD's */
            setup_timeout(&tv, timeout_ms);
            FD_ZERO(&fdset);
            FD_SET(sock->fd, &fdset);
#endif /* !WOLFMQTT_NO_TIMEOUT */

#if !defined(WOLFMQTT_NO_TIMEOUT) && defined(WOLFMQTT_NONBLOCK)
            if (mqttCtx->useNonBlockMode) {
                /* Set socket as non-blocking */
                tcp_set_nonblocking(&sock->fd);
            }
#endif

            /* Start connect */
            rc = SOCK_CONNECT(sock->fd, (struct sockaddr*)&sock->addr,
                              sizeof(sock->addr));
            if (rc < 0) {
                /* Check for error */
                socklen_t len = sizeof(so_error);
                getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

                /* set default error case */
                rc = MQTT_CODE_ERROR_NETWORK;
#ifdef WOLFMQTT_NONBLOCK
                if (errno == EINPROGRESS || so_error == EINPROGRESS) {
#ifndef WOLFMQTT_NO_TIMEOUT
                    /* Wait for connect */
                    if (select((int)SELECT_FD(sock->fd), NULL, &fdset, NULL,
                               &tv) > 0) {
                        rc = MQTT_CODE_SUCCESS;
                    }
#else
                    rc = MQTT_CODE_CONTINUE;
#endif
                }
#endif
            }
            break;
        }

        default:
            rc = -1;
    } /* switch */

    (void)timeout_ms;

exit:
    /* Show error */
    if (rc != 0) {
        PRINTF("NetConnect: Rc=%d, SoErr=%d", rc, so_error);
    }

    return rc;
}

static int NetWrite(void* context, const byte* buf, int buf_len,
                    int timeout_ms) {
    SocketContext* sock = (SocketContext*)context;
    int rc;
    SOERROR_T so_error = 0;
#ifndef WOLFMQTT_NO_TIMEOUT
    struct timeval tv;
#endif

    if (context == NULL || buf == NULL || buf_len <= 0) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    if (sock->fd == SOCKET_INVALID) return MQTT_CODE_ERROR_BAD_ARG;

#ifndef WOLFMQTT_NO_TIMEOUT
    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);
    setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
#endif

    rc = (int)SOCK_SEND(sock->fd, buf, buf_len, 0);
    if (rc == -1) {
        /* Get error */
        socklen_t len = sizeof(so_error);
        getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error == 0) {
            rc = 0; /* Handle signal */
        } else {
#ifdef WOLFMQTT_NONBLOCK
            if (so_error == EWOULDBLOCK || so_error == EAGAIN) {
                return MQTT_CODE_CONTINUE;
            }
#endif
            rc = MQTT_CODE_ERROR_NETWORK;
            PRINTF("NetWrite: Error %d", so_error);
        }
    }

    (void)timeout_ms;

    return rc;
}

static int NetRead_ex(void* context, byte* buf, int buf_len, int timeout_ms,
                      byte peek) {
    SocketContext* sock = (SocketContext*)context;
    MQTTCtx_t* mqttCtx = sock->mqttCtx;
    int rc = -1, timeout = 0;
    SOERROR_T so_error = 0;
    int bytes = 0;
    int flags = 0;
#ifndef WOLFMQTT_NO_TIMEOUT
    fd_set recvfds;
    fd_set errfds;
    struct timeval tv;
#endif

    (void)mqttCtx;

    if (context == NULL || buf == NULL || buf_len <= 0) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    if (sock->fd == SOCKET_INVALID) return MQTT_CODE_ERROR_BAD_ARG;

    if (peek == 1) {
        flags |= MSG_PEEK;
    }

#ifndef WOLFMQTT_NO_TIMEOUT
    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);

    /* Setup select file descriptors to watch */
    FD_ZERO(&errfds);
    FD_SET(sock->fd, &errfds);
    FD_ZERO(&recvfds);
    FD_SET(sock->fd, &recvfds);
#ifdef WOLFMQTT_ENABLE_STDIN_CAP
#ifdef WOLFMQTT_MULTITHREAD
    FD_SET(sock->pfd[0], &recvfds);
#endif
    if (!mqttCtx->test_mode) {
        FD_SET(STDIN, &recvfds);
    }
#endif /* WOLFMQTT_ENABLE_STDIN_CAP */
#else
    (void)timeout_ms;
#endif /* !WOLFMQTT_NO_TIMEOUT */

    /* Loop until buf_len has been read, error or timeout */
    while (bytes < buf_len) {
        int do_read = 0;

#ifndef WOLFMQTT_NO_TIMEOUT
#ifdef WOLFMQTT_NONBLOCK
        if (mqttCtx->useNonBlockMode) {
            do_read = 1;
        } else
#endif
        {
            /* Wait for rx data to be available */
            rc = select((int)SELECT_FD(sock->fd), &recvfds, NULL, &errfds, &tv);
            if (rc > 0) {
                if (FD_ISSET(sock->fd, &recvfds)) {
                    do_read = 1;
                }
                if (FD_ISSET(sock->fd, &errfds)) {
                    rc = -1;
                    break;
                }
            } else {
                timeout = 1;
                break; /* timeout or signal */
            }
        }
#else
        do_read = 1;
#endif /* !WOLFMQTT_NO_TIMEOUT */

        if (do_read) {
            /* Try and read number of buf_len provided,
                minus what's already been read */
            rc = (int)SOCK_RECV(sock->fd, &buf[bytes], buf_len - bytes, flags);
            if (rc <= 0) {
                rc = -1;
                goto exit; /* Error */
            } else {
                bytes += rc; /* Data */
            }
        }

        /* no timeout and non-block should always exit loop */
#ifdef WOLFMQTT_NONBLOCK
        if (mqttCtx->useNonBlockMode) {
            break;
        }
#endif
#ifdef WOLFMQTT_NO_TIMEOUT
        break;
#endif
    } /* while */

exit:

    if (rc == 0 && timeout) {
        rc = MQTT_CODE_ERROR_TIMEOUT;
    } else if (rc < 0) {
        /* Get error */
        socklen_t len = sizeof(so_error);
        getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

        if (so_error == 0) {
            rc = 0; /* Handle signal */
        } else {
#ifdef WOLFMQTT_NONBLOCK
            if (so_error == EWOULDBLOCK || so_error == EAGAIN) {
                return MQTT_CODE_CONTINUE;
            }
#endif
            rc = MQTT_CODE_ERROR_NETWORK;
            PRINTF("NetRead: Error %d", so_error);
        }
    } else {
        rc = bytes;
    }

    return rc;
}

static int NetRead(void* context, byte* buf, int buf_len, int timeout_ms) {
    return NetRead_ex(context, buf, buf_len, timeout_ms, 0);
}

static int NetDisconnect(void* context) {
    SocketContext* sock = (SocketContext*)context;
    if (sock) {
        if (sock->fd != SOCKET_INVALID) {
            SOCK_CLOSE(sock->fd);
            sock->fd = -1;
        }

        sock->stat = SOCK_BEGIN;
    }
    return 0;
}
#endif

/* Public Functions */
int MqttClientNet_Init(MqttNet* net, MQTTCtx_t* mqttCtx) {
#if defined(USE_WINDOWS_API) && !defined(FREERTOS_TCP)
    WSADATA wsd;
    WSAStartup(0x0002, &wsd);
#endif

    if (net) {
        SocketContext* sockCtx;

        XMEMSET(net, 0, sizeof(MqttNet));
        net->connect = NetConnect;
        net->read = NetRead;
        net->write = NetWrite;
        net->disconnect = NetDisconnect;

        sockCtx = (SocketContext*)WOLFMQTT_MALLOC(sizeof(SocketContext));
        if (sockCtx == NULL) {
            return MQTT_CODE_ERROR_MEMORY;
        }
        net->context = sockCtx;
        XMEMSET(sockCtx, 0, sizeof(SocketContext));
        sockCtx->fd = SOCKET_INVALID;
        sockCtx->stat = SOCK_BEGIN;
        sockCtx->mqttCtx = mqttCtx;
    }

    return MQTT_CODE_SUCCESS;
}

int MqttClientNet_DeInit(MqttNet* net) {
    if (net) {
        if (net->context) {
            WOLFMQTT_FREE(net->context);
        }
        XMEMSET(net, 0, sizeof(MqttNet));
    }
    return 0;
}

int MqttClientNet_Wake(MqttNet* net) {
#if defined(WOLFMQTT_MULTITHREAD) && defined(WOLFMQTT_ENABLE_STDIN_CAP)
    if (net) {
        SocketContext* sockCtx = (SocketContext*)net->context;
        if (sockCtx) {
            /* wake the select() */
            if (write(sockCtx->pfd[1], "\n", 1) < 0) {
                PRINTF("Failed to wake select");
                return -1;
            }
        }
    }
#else
    (void)net;
#endif
    return 0;
}
