#include <easyflash.h>
#include <fal.h>
#include <dfs_posix.h>
#include "dfs_fs.h"
#include "W601_app.h"
#include "time.h"
w601_t w601;

//调试选项
#define DBG_SECTION_NAME "W601_APP"
#include "w601_debug.h"
void telnet_server(void);

/**
* Name:    w601_mutex_init
* Brief:   信号量初始化
* Input:   None
* Return:  Success: 0   Fail: -1
*/
int w601_mutex_init(void)
{
    rt_mutex_t sensor_mutex = rt_mutex_create("sensor", RT_IPC_FLAG_FIFO);
    if (sensor_mutex == NULL)
    {
        LOG_E("sensor mutex create fail");
        goto error;
    }
    else
    {
        w601.mutex.sensor_mutex = sensor_mutex;
        return 0;
    }
error:
    return -1;
}

/**
* Name:    w601_filesystem_init
* Brief:   初始化文件系统
* Input:   None
* Return:  Success:0    Fail:-1 
*/
int w601_filesystem_init(void)
{
    //flash设备
    //struct rt_device *flash_dev = RT_NULL;
    const struct fal_partition *part = RT_NULL;

    if (fal_init() < 0)
    {
        return -1;
    }
    else
    {

        part = fal_partition_find("download");
        fal_partition_erase_all(part);

        //挂载 spi flash 中名为 "filesystem" 的分区
        if (dfs_mount("filesystem", "/", "elm", 0, 0) < 0)
        {
            //若挂载失败则格式化块设备并重新挂载
            LOG_E("mount filesystem fail");
            LOG_W("filesystem auto formating ...");
            dfs_mkfs("elm", "filesystem");
            rt_thread_mdelay(1000);
            return -1;
        }
        LOG_I("filesystem mount success");
    }
    return 0;
}

static void log_software_version(void)
{
    rt_thread_mdelay(100);
    rt_kprintf("\r\n");
    rt_kprintf("*************************************************\r\n");
    rt_thread_mdelay(100);
    rt_kprintf("system running ...\r\n");
    rt_thread_mdelay(100);
    rt_kprintf("software version: %s\r\n", SOFTWARE_VERSION);
    rt_thread_mdelay(100);
    rt_kprintf("*************************************************\r\n");
    rt_thread_mdelay(100);
    printf("\r\n");
}

/**
 * Name:    w601_app_init
 * Brief:   w601应用初始化
 * Input:   None
 * Return:  None
 */
void w601_app_init(void)
{
    log_software_version();
    w601_filesystem_init();
    easyflash_init();
    wifi_start();
    ntp_sync_to_rtc(NULL); //系统校时
    w601_mutex_init();
    

    //W601模组初始化
    aht10_module_init();
    web_module_init();
    led_module_init();
    smtp_module_init();
    ap3216c_module_init();
    telnet_server();
    
    rt_thread_mdelay(10000);
    record_module_init();
}
