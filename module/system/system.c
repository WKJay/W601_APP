/***************************************************************
 * @file           system.c
 * @brief
 * @author         WKJay
 * @Version
 * @Date           2022-11-18
 ***************************************************************/
#include <rtthread.h>
#include <rthw.h>
#include <stdint.h>
#include "system.h"

static struct _sys_mem {
    uint32_t max_mem;
    uint32_t used_mem;
    uint32_t max_used_mem;
} sys_mem;

static uint8_t reset = 0;

/**
 * Name:          mem_monitor
 * Brief:         内存监控
 * Input:         None
 * Output:        None
 */
static void mem_monitor(void) {
    uint8_t used_per = 0;

    rt_memory_info((rt_uint32_t *)&sys_mem.max_mem, (rt_uint32_t *)&sys_mem.used_mem,
                   (rt_uint32_t *)&sys_mem.max_used_mem);

    if (used_per > 95) {
        rt_hw_cpu_reset();
    }
}

/**
 * Name:    sys_reset_check
 * Brief:   系统重启监测
 * Input:   None
 * Output:  None
 */
static void sys_reset_check(void) {
    if (reset == 1) {
        rt_hw_cpu_reset();
    }
}

/**
 * Name:    system_monitor_thread
 * Brief:   系统监控线程
 * Input:   None
 * Output:  None
 */
static void system_monitor_thread(void *param) {
    while (1) {
        mem_monitor();
        sys_reset_check();
        rt_thread_mdelay(500);
    }
}

void system_reset(void) { reset = 1; }

/**
 * Name:          system_monitor_create
 * Brief:         创建系统监控线程
 * Input:         none
 * Output:        none
 */
void system_monitor_create(void) {
    rt_thread_t tid;
    tid = rt_thread_create("sys_mon", system_monitor_thread, RT_NULL, 2048,
                           RT_THREAD_PRIORITY_MAX - 3, 10);
    RT_ASSERT(tid != RT_NULL);

    rt_thread_startup(tid);
}
