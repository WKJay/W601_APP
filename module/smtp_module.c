#include "smtp_module.h"
#include "wifi_module.h"
#include "smtp_client.h"
#include "aht10_module.h"
#include "w601_app.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rtthread.h"
#include "cJSON.h"

#ifdef SMTP_CLIENT_USING_TLS
#define SMTP_CLIENT_THREAD_STACK_SIZE 4096
#else
#define SMTP_CLIENT_THREAD_STACK_SIZE 2048
#endif

#define LOG_TAG "smtp"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

//报警冷却时间（单位：s）
#define WARNING_COLD_TIME 300

smtp_module_t w601_smtp;

/*
 *邮件信息相关宏定义
 */
//smtp 服务器域名
#define SMTP_SERVER_ADDR "smtp.qq.com"
//smtp 服务器端口号
#define SMTP_SERVER_PORT "25"
//smtp 登录用户名
#define SMTP_USERNAME ""
//smtp 登录密码（或凭证）
#define SMTP_PASSWORD ""
//邮件主题
#define SMTP_SUBJECT "W601 MONITOR WARNING"

//邮件内容
char content[1024] = "";

//准备邮件内容
static void prepare_content(void)
{
    sprintf(content, "W601监测系统报警\r\n-------------------------\r\n当前温度：%.2f C\r\n当前湿度：%.2f%% \r\n",
                                    w601.aht10_data.cur_temp,w601.aht10_data.cur_humi);
}

void smtp_thread(void *param)
{
    //初始化smtp客户端
    smtp_client_init();
    //清空设置
    rt_memset(&w601_smtp, 0, sizeof(smtp_module_t));

    while (1)
    {
        if (w601_smtp.cold_time)
        {
            w601_smtp.cold_time--;
        }
            
        if (w601_smtp.enable) //使能邮件功能
        {
            if (w601.aht10_data.cur_temp > w601.aht10_data.temp_warn) //有报警
            {
                //冷却时间内不重复报警
                if (w601_smtp.cold_time)
                {
                    
                }
                else
                {
                    //设置报警冷却时间
                    w601_smtp.cold_time = WARNING_COLD_TIME;
                    LOG_W("system warning,smtp send!");
                    prepare_content();
                    smtp_send_mail(SMTP_SUBJECT, content);
                }
            }
        }
        rt_thread_mdelay(1000);
    }
}

int smtp_data_config(char *server_addr, char *server_port,
                     char *username, char *password,
                     char *receiver, char *temp_warn)
{
    if (w601_smtp.receiver[0] != '\0')
    {
        smtp_delete_receiver(w601_smtp.receiver);
    }
    strcpy(w601_smtp.server_addr, server_addr);
    strcpy(w601_smtp.server_port, server_port);
    strcpy(w601_smtp.username, username);
    strcpy(w601_smtp.password, password);
    strcpy(w601_smtp.receiver, receiver);

    smtp_set_server_addr(w601_smtp.server_addr,
                         ADDRESS_TYPE_DOMAIN,
                         w601_smtp.server_port);

    smtp_set_auth(w601_smtp.username, w601_smtp.password);
    smtp_add_receiver(receiver);

    w601.aht10_data.temp_warn = atof(temp_warn);

    return 0;
}

void smtp_enable(void)
{
    w601_smtp.enable = 1;
}

void smtp_disable(void)
{
    w601_smtp.enable = 0;
}

char *json_create_smtp_data(void)
{
    char *json_data = RT_NULL;
    char value[10] = "";
    cJSON *root = cJSON_CreateObject();
    snprintf(value, sizeof(value), "%.2f", w601.aht10_data.temp_warn);

    cJSON_AddItemToObject(root, "server_addr", cJSON_CreateString(w601_smtp.server_addr));
    cJSON_AddItemToObject(root, "server_port", cJSON_CreateString(w601_smtp.server_port));
    cJSON_AddItemToObject(root, "username", cJSON_CreateString(w601_smtp.username));
    cJSON_AddItemToObject(root, "password", cJSON_CreateString(w601_smtp.password));
    cJSON_AddItemToObject(root, "receiver", cJSON_CreateString(w601_smtp.receiver));
    cJSON_AddItemToObject(root, "temp_warn", cJSON_CreateString(value));
    cJSON_AddItemToObject(root, "enable", cJSON_CreateNumber(w601_smtp.enable));

    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}

int smtp_module_init(void)
{
    rt_thread_t smtp_client_tid;
    //创建邮件发送线程（如果选择在主函数中直接调用邮件发送函数，需要注意主函数堆栈大小，必要时调大）
    smtp_client_tid = rt_thread_create("smtp", smtp_thread, RT_NULL, SMTP_CLIENT_THREAD_STACK_SIZE, 20, 5);
    if (smtp_client_tid != RT_NULL)
    {
        rt_thread_startup(smtp_client_tid);
    }
    return RT_EOK;
}
