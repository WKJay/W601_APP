/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     web_utils.c
 Description:
 History:
 1. Version:
    Date:       2021-01-29
    Author:     WKJay
    Modify:
*************************************************/
#include <rtthread.h>
#include <dfs_posix.h>
#include "wn_module.h"
#include "web_utils.h"

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "WEB-UTILS"
#define DBG_LEVEL        DBG_INFO
#include <rtdbg.h>

#define WEB_INDEX_PATH ("/webnet/$upload$.html")
extern const char upload_page[];
extern int get_upload_page_size(void);

/**
 * Name:    web_file_init
 * Brief:   初始化网页文件
 * Input:   None
 * Output:
 *  成功：0
 *  失败：-1
 */
int web_file_init(void) {
    FILE *fp = RT_NULL;
    check_create_path(WEBNET_ROOT);
    fp = fopen(WEB_INDEX_PATH, "w+b");
    if (fp == NULL) {
        LOG_E("open web index html fail");
        return -1;
    }
    if (fwrite(upload_page, get_upload_page_size(), 1, fp) != 1) {
        LOG_E("write web index html fail");
        fclose(fp);
        return -1;
    } else {
        LOG_I("write web index html success");
        fclose(fp);
        return 0;
    }
}

static char *_str_normalize_path(char *fullpath) {
    char *dst0, *dst, *src;

    src = fullpath;
    dst = fullpath;

    dst0 = dst;
    while (1) {
        char c = *src;

        if (c == '.') {
            if (!src[1])
                src++; /* '.' and ends */
            else if (src[1] == '/') {
                /* './' case */
                src += 2;

                while ((*src == '/') && (*src != '\0')) src++;
                continue;
            } else if (src[1] == '.') {
                if (!src[2]) {
                    /* '..' and ends case */
                    src += 2;
                    goto up_one;
                } else if (src[2] == '/') {
                    /* '../' case */
                    src += 3;

                    while ((*src == '/') && (*src != '\0')) src++;
                    goto up_one;
                }
            }
        }

        /* copy up the next '/' and erase all '/' */
        while ((c = *src++) != '\0' && c != '/') *dst++ = c;

        if (c == '/') {
            *dst++ = '/';
            while (c == '/') c = *src++;

            src--;
        } else if (!c)
            break;

        continue;

    up_one:
        dst--;
        if (dst < dst0) return RT_NULL;
        while (dst0 < dst && dst[-1] != '/') dst--;
    }

    *dst = '\0';

    /* remove '/' in the end of path if exist */
    dst--;
    if ((dst != fullpath) && (*dst == '/')) *dst = '\0';

    return fullpath;
}

static int _check_or_create_directory(char *dir_path) {
    DIR *dir = NULL;
    dir = opendir(dir_path);
    if (dir == NULL) {
        rt_kprintf("creating directory:%s\r\n", dir_path);
        if (mkdir(dir_path, 0x777) < 0) {
            rt_kprintf("create directory:%s failed.\r\n", dir_path);
            return -1;
        } else {
            return 0;
        }
    } else {
        closedir(dir);
    }
    return 0;
}

int relative_path_2_absolute(char *path_from, char *path_to, int to_length,
                             char *absolute_prefix) {
    int max_length = 0;
    // absolute prefix + '/' + origin path length + '\0'
    max_length = rt_strlen(path_from) + rt_strlen(absolute_prefix) + 1 + 1;
    if (max_length > to_length) {
        rt_kprintf("path buffer too small\r\n");
        return -1;
    }
    rt_memset(path_to, 0, to_length);
    rt_snprintf(path_to, to_length, "%s/%s", absolute_prefix, path_from);
    _str_normalize_path(path_to);
    return 0;
}

int create_file_by_path(char *path) {
    int fd = -1;
    int length = 0;
    char path_buff[MAX_FILENAME_LENGTH];
    rt_memset(path_buff, 0, sizeof(path_buff));

    length = strlen(path);
    for (int i = 0; i < length; i++) {
        path_buff[i] = path[i];

        if (path[i + 1] == '/') {
            if (_check_or_create_directory(path_buff) < 0) {
                return -1;
            }
        } else if (path[i + 1] == '\0' && path[i] != '/') {
            fd = open(path_buff, O_CREAT | O_RDWR);
            rt_kprintf("write file %s\r\n", path_buff);
            if (fd > 0) {
                return fd;
            } else {
                rt_kprintf("%s creates failed\r\n", path_buff);
            }
        }
    }
    return -1;
}
