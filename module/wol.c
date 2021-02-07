/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     wol.c
 Description:
 History:
 1. Version:
    Date:       2021-02-06
    Author:     WKJay
    Modify:
*************************************************/
#include <rtthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#define WOL_PROCESS_LOG_PATH "/webnet/wol_log.txt"
#define MAGIC_OFFSET         6

int _wol(rt_uint8_t *mac) {
    int sock = -1;
    struct sockaddr_in server_addr;
    uint8_t magic_data[102];
    rt_memset(magic_data, 0xff, sizeof(magic_data));

    for (int i = 0; i < 16; i++) {
        rt_memcpy(&magic_data[MAGIC_OFFSET + i * 6], mac, 6);
    }

    /* 创建一个 socket，类型是 SOCK_DGRAM，UDP 类型 */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        rt_kprintf("Socket error\n");
        return -1;
    }
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));
    /* 初始化预连接的服务端地址 */
		
	rt_memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7);
    server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if (sendto(sock, magic_data, sizeof(magic_data), 0,
               (struct sockaddr *)&server_addr,
               sizeof(struct sockaddr)) != sizeof(magic_data)) {
								 rt_kprintf("sendto sock:%d error\n",sock);
        return -1;
    }

    rt_thread_mdelay(1000);

    closesocket(sock);
    return 0;
}

int wol_process(const char *value) {
    unsigned int mac[6];
    rt_uint8_t mac_u8[6];
    rt_memset(mac, 0, sizeof(mac));

    if (sscanf(value, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3],
               &mac[4], &mac[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac_u8[i] = (rt_uint8_t)mac[i];
        }
        if (_wol(mac_u8) == 0) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}
