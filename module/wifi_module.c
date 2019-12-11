#include "rtthread.h"
#include <rtdevice.h>
#include "drv_wifi.h"
#include "wifi_config.h"
#include <msh.h>

#define DBG_TAG "wifi"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define WLAN_SSID "人才"
#define WLAN_PASSWORD "qwertyuiop"

static uint8_t wifi_ready_status = 0;//wifi连接状态

//连接超时时间
#define NET_READY_TIME_OUT (rt_tick_from_millisecond(15 * 1000))

//wifi连接成功信号量
static struct rt_semaphore net_ready;


uint8_t wifi_ready_status_get(void)
{
    return wifi_ready_status;
}

//wifi连接成功回调
static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    wifi_ready_status = 1;
    rt_sem_release(&net_ready);
}

// 断开连接回调函数
static void wlan_station_disconnect_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    wifi_ready_status = 0;
}

int wifi_start(void)
{
    int result = -1;
    //配置WIFI为STATION模式
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    rt_thread_mdelay(100);
    rt_sem_init(&net_ready, "net_ready", 0, RT_IPC_FLAG_FIFO);
    // 注册 wlan ready 回调函数
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);
    // 注册 wlan 断开回调函数
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wlan_station_disconnect_handler, RT_NULL);
    //wifi连接
    result = rt_wlan_connect(WLAN_SSID, WLAN_PASSWORD);
    if (result == RT_EOK)
    {
        /* 等待成功获取 IP */
        result = rt_sem_take(&net_ready, NET_READY_TIME_OUT);
        if (result == RT_EOK)
        {
            LOG_D("networking ready!");
            msh_exec("ifconfig", rt_strlen("ifconfig"));
        }
        else
        {
            LOG_D("wait ip got timeout!");
        }
        /* 回收资源 */
        rt_wlan_unregister_event_handler(RT_WLAN_EVT_READY);
        rt_sem_detach(&net_ready);
    }
    else
    {
        LOG_E("The AP(%s) is connect failed!", WLAN_SSID);
    }
    /* 初始化自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    return 0;
}
