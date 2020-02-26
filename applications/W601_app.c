#include <easyflash.h>
#include <fal.h>
#include <dfs_posix.h>
#include "dfs_fs.h"
#include "w601_module.h"

//调试选项
#define DBG_SECTION_NAME "W601_APP"
#include "w601_debug.h"

/**
* Name:    w601_filesystem_init
* Brief:   初始化文件系统
* Input:   None
* Return:  Success:0    Fail:-1 
*/
int w601_filesystem_init(void)
{
    //flash设备
    struct rt_device *flash_dev = RT_NULL;

    if (fal_init() < 0)
    {
        return -1;
    }
    else
    {
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

/**
 * Name:    w601_app_init
 * Brief:   w601应用初始化
 * Input:   None
 * Return:  None
 */
void w601_app_init(void)
{
    w601_filesystem_init();
    easyflash_init();
    wifi_start();

    //W601模组初始化
    aht10_module_init();
    web_module_init();
    led_module_init();
    smtp_module_init();
}
