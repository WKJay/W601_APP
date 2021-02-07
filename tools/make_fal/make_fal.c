/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs;:
 * Date           Author       Notes
 * 2019-09-09     tyx          first implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "crc32.h"

#define SECBOOT_IMG_ADDR        (0X2100)
#define SECBOOT_HEADER_LEN      (0X100)
#define SECBOOT_IMG_TOTAL_LEN   (1024 * 56)
#define SECBOOT_MAGIC_NO        (0XA0FFFF9F)

#define FLS_FILE_MAX_SIZE       (1024 * 1024 * 4)
#define FAL_FILE_MAX_SIZE       (1024 * 4)

struct img_head
{
    uint32_t magic;
    uint16_t img_type;
    uint16_t zip_type;
    uint32_t app_addr;
    uint32_t app_len;
    uint32_t head_crc;
    uint32_t img_addr;
    uint32_t img_len;
    uint32_t img_crc;
    uint32_t img_no;
    uint8_t ver[16];
};

static int file_read(const char *path, void *buff, int len)
{
    FILE *file;
    int f_len;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }

    f_len = fseek(file, 0, SEEK_END);
    if (f_len > len)
    {
        return 0;
    }

    fseek(file, 0, SEEK_SET);
    f_len = fread(buff, 1, len, file);
    fclose(file);

    return f_len;
}

static int file_write(const char *path, void *buff, int len)
{
    FILE *file;
    int f_len;

    file = fopen(path, "wb+");
    if (file == NULL)
    {
        return 0;
    }

    f_len = fwrite(buff, 1, len, file);
    fclose(file);

    return f_len;
}

int main(int argc, char *argv[])
{
    const char *p_fls = NULL;
    const char *p_fal = NULL;
    const char *p_out = NULL;
    uint8_t *m_fls = NULL;
    uint8_t *m_fal = NULL;
    uint8_t *m_out = NULL;
    int s_fls, s_fal, s_out;
    struct img_head *head;
    struct crc32_cfg cfg;
    uint32_t crc_val;

    if (argc == 4)
    {
        p_fls = argv[1];
        p_fal = argv[2];
        p_out = argv[3];
    }
    else if (argc == 3)
    {
        p_fls = argv[1];
        p_fal = argv[2];
        p_out = argv[1];
    }
    else
    {
        printf("param cnt error!\n");
        printf("param 0: .\n");
        printf("param 1: input FLS file\n");
        printf("param 2: input FAL file\n");
        printf("param 3: output FLS file\n");
        return 0;
    }

    /* open and read fls file to mem */
    m_fls = calloc(1, FLS_FILE_MAX_SIZE);
    if (m_fls == NULL)
    {
        printf("err:there is not enough memory! exit.\n");
        goto out;
    }

    s_fls = file_read(p_fls, m_fls, FLS_FILE_MAX_SIZE);
    if (s_fls == 0)
    {
        printf("err:open FLS file failed! path:%s\n", p_fls);
        goto out;
    }

    /* open and read fal file to mem */
    m_fal = calloc(1, FAL_FILE_MAX_SIZE);
    if (m_fal == NULL)
    {
        printf("err:there is not enough memory! exit.\n");
        goto out;
    }
    s_fal = file_read(p_fal, m_fal, FAL_FILE_MAX_SIZE);
    if (s_fal == 0)
    {
        printf("err:open FAL file failed! path:%s\n", p_fls);
        goto out;
    }

    /* fls file check */
    head = (struct img_head *)m_fls;
    if (head->magic != SECBOOT_MAGIC_NO)
    {
        printf("err:This is not a FLS file!\n");
        goto out;
    }

    /* create out file */
    s_out = s_fls;
    m_out = calloc(1, s_out);
    if (m_out == NULL)
    {
        printf("err:there is not enough memory! exit.\n");
        goto out;
    }

    memcpy(m_out, m_fls, s_fls);
    memcpy(&m_out[SECBOOT_IMG_TOTAL_LEN - s_fal], m_fal, s_fal);

    /* crc32 */
    memset(&cfg, 0, sizeof(cfg));
    cfg.val = 0xffffffff;
    cfg.xorout = 0;
    cfg.flags = 0;
    crc_val = crc32(&cfg, &m_out[56], s_out - 56);

    head = (struct img_head *)m_out;

    head->magic = SECBOOT_MAGIC_NO;
    head->img_type = 0;
    head->zip_type = 0;
    head->app_addr = SECBOOT_IMG_ADDR - SECBOOT_HEADER_LEN;
    head->app_len = s_out - 56;
    head->head_crc = crc_val;
    head->img_addr = 0;
    head->img_len = 0;
    head->img_crc = 0;
    head->img_no = 0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.val = 0xffffffff;
    cfg.xorout = 0;
    cfg.flags = 0;
    crc_val = crc32(&cfg, head, sizeof(*head));

    *(uint32_t *)& head[1] = crc_val;

    /* output file */
    if (file_write(p_out, m_out, s_out) != s_out)
    {
        printf("err:output file failed\n path:%s", p_out);
    }

out:
    free(m_fls);
    free(m_fal);
    free(m_out);
    return 0;
}
