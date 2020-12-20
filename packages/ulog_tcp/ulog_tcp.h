#ifndef __ULOG_TCP_H
#define __ULOG_TCP_H

int ulog_tcp_init(void);
int ulog_tcp_add_server(unsigned char *ip, unsigned short port);
int ulog_tcp_delete_server(uint8_t *ip, uint16_t port);

#endif /* __ULOG_TCP_H */
