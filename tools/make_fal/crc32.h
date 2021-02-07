/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs;:
 * Date           Author       Notes
 * 2019-09-09     tyx          first implementation
 */

#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

#define CRC_FLAG_REFIN    (0x1 << 0)
#define CRC_FLAG_REFOUT   (0x1 << 1)

struct crc32_cfg
{
    uint32_t val;            /**< Last CRC value cache */
    uint32_t xorout;         /**< Result XOR Value */
    uint32_t flags;          /**< Input or output data reverse. CRC_FLAG_REFIN or CRC_FLAG_REFOUT */
};

uint32_t crc32(struct crc32_cfg *cfg, void *buff, int len);

#endif
