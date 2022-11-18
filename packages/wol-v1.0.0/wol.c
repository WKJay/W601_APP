/***************************************************************
 * @file           wol.c
 * @brief
 * @author         WKJay
 * @Version
 * @Date           2022-11-16
 ***************************************************************/
#include <rtthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include "wol.h"

#define MAGIC_OFFSET 6
#define SOCK_PORT    7

static int do_wol(uint8_t *mac) {
    int sock = -1;
    int index = 0;
    int optval = 1;
    struct sockaddr_in server_addr;
    uint8_t magic_data[102];

    rt_memset(magic_data, 0xff, sizeof(magic_data));

    for (index = 0; index < 16; index++) {
        rt_memcpy(&magic_data[MAGIC_OFFSET + index * 6], mac, 6);
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        rt_kprintf("create socket error\r\n");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));

    rt_memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SOCK_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if (sendto(sock, magic_data, sizeof(magic_data), 0, (struct sockaddr *)&server_addr,
               sizeof(struct sockaddr)) != sizeof(magic_data)) {
        rt_kprintf("send wol data failed.\r\n");
        return -1;
    }

    rt_thread_mdelay(1000);

    closesocket(sock);

    return 0;
}

int wol(uint8_t *mac) { return do_wol(mac); }

int wol_str(char *mac_str) {
    int ret = -1;
    uint8_t mac[6] = {0};
    ret = sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3],
                 &mac[4], &mac[5]);
    if (ret == 6) goto next;
    ret = sscanf(mac_str, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx", &mac[0], &mac[1], &mac[2], &mac[3],
                 &mac[4], &mac[5]);
    if (ret == 6) goto next;
    ret = sscanf(mac_str, "%hhx %hhx %hhx %hhx %hhx %hhx", &mac[0], &mac[1], &mac[2], &mac[3],
                 &mac[4], &mac[5]);
    if (ret == 6) goto next;

    rt_kprintf("invalid mac: %s\r\n", mac);
    return -1;
next:
    return do_wol(mac);
}

static void wol_wakeup(int argc, char *argv[]) {
    if (argc != 2) {
        rt_kprintf("invalid arguments,please input:\r\n");
        rt_kprintf("wol_wakeup 00:01:02:03:04:05 or\r\n");
        rt_kprintf("wol_wakeup 00-01-02-03-04-05 or\r\n");
        rt_kprintf("wol_wakeup \"00 01 02 03 04 05\"\r\n");
        return;
    }
    if (wol_str(argv[1]) < 0) {
        rt_kprintf("wake %s failed.", argv[1]);
        return;
    }
    return;
}
MSH_CMD_EXPORT(wol_wakeup, Wake on Lan)
