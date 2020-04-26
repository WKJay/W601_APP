/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-15     yangjie      add sensor port file
 */

#include <rtthread.h>

#ifdef BSP_USING_AHT10
#include "sensor_asair_aht10.h"

#define AHT10_I2C_BUS  "i2c1soft"

int rt_hw_aht10_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name  = AHT10_I2C_BUS;
    cfg.intf.user_data = (void *)AHT10_I2C_ADDR;

    rt_hw_aht10_init("aht10", &cfg);

    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_aht10_port);

#endif

#ifdef PKG_USING_AP3216C
#include "sensor_lsc_ap3216c.h"

#define AP3216C_I2C_BUS  "i2c2soft"

int rt_hw_ap3216c_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name  = AP3216C_I2C_BUS;
    cfg.intf.user_data = (void *)AP3216C_I2C_ADDR;

    rt_hw_ap3216c_init("ap3216c", &cfg);

    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_ap3216c_port);
#endif