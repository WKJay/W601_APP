#ifndef __SMTP_MODULE_H
#define __SMTP_MODULE_H

#include <stdint.h>

typedef struct
{
    uint8_t enable;        //邮件功能使能
    uint32_t cold_time;    //报警冷却时间
    char server_addr[100]; //邮件服务器地址
    char server_port[10];  //邮件服务器端口
    char username[50];     //用户名
    char password[50];     //密码
    char receiver[100];    //收件人地址
} smtp_module_t;

int smtp_module_init(void);
int smtp_data_config(char *server_addr, char *server_port,
                     char *username, char *password,
                     char *receiver, char *temp_warn);

char *json_create_smtp_data(void);
void smtp_enable(void);
void smtp_disable(void);

#endif /*__SMTP_MODULE_H*/
