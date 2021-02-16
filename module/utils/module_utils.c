/*************************************************
 Copyright (c) 2020
 All rights reserved.
 File name:     utils.c
 Description:
 History:
 1. Version:
    Date:       2021-02-10
    Author:     WKJay
    Modify:
*************************************************/
#include "module_utils.h"
#include <dfs_posix.h>
#include <string.h>

#define DISK_MIN_FREE_SPACE_KB 512
#define FS_PARTITION_NAME      "filesystem"

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "util"
#define DBG_LEVEL        DBG_INFO
#include <rtdbg.h>

void safe_free(void *ptr) {
    if (ptr) rt_free(ptr);
}

//读取文件数据，需注意使用后需要释放空间
uint8_t *file_read(char *path) {
    FILE *fp;
    int fp_size;
    uint8_t *buf;

    fp = fopen(path, "rb");
    if (!fp) {
        LOG_E("cannot find config file: %s!", path);
        return NULL;
    }
    // calculate file size
    fseek(fp, 0, SEEK_END);
    fp_size = ftell(fp);
    rewind(fp);

    LOG_D("file size is %d", fp_size);
    // read file
    buf = malloc(sizeof(char) * fp_size);
    if (buf == NULL) {
        LOG_E("file buffer malloc error!");
        fclose(fp);
        return NULL;
    }

    int result = fread(buf, 1, fp_size, fp);
    if (result != fp_size) {
        LOG_E("file read error");
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return buf;
}

//获取剩余存储空间（单位：KB）
long long get_disk_free(void) {
    struct statfs disk_s;
    int res;
    long long cap;

    res = dfs_statfs("/", &disk_s);
    if (res != 0) {
        rt_kprintf("dfs_statfs failed.\n");
        return -1;
    }

    cap = ((long long)disk_s.f_bsize) * ((long long)disk_s.f_bfree) / 1024LL;
    return cap;
}

//检查剩余存储空间容量，若过小则会格式化
int check_disk_free(void) {
    LOG_D("checking disk free...!");
    if (get_disk_free() < DISK_MIN_FREE_SPACE_KB) {
        LOG_W("disk full , erasing all content...");
        if (dfs_mkfs("elm", FS_PARTITION_NAME) < 0) {
            LOG_E("erase fail,reboot device");

        } else {
            LOG_I("erase success");
        }
    } else {
        LOG_I("get enough disk free space.");
    }

    return 0;
}

int check_create_path(const char *path) {
    DIR *dirp = NULL;
    dirp = opendir(path);
    if (dirp) {
        closedir(dirp);
    } else {
        LOG_W("no dir:%s,create one", path);
        if (mkdir(path, 0x777) < 0) {
            LOG_E("create dir %s failed", path);
            return -1;
        }
        /* check again */
        dirp = opendir(path);
        if (dirp) {
            closedir(dirp);
        } else {
            LOG_E("create dir %s failed", path);
            return -1;
        }
    }
    return 0;
}

int str_compare(char *str1, char *str2) {
    if (strlen(str1) != strlen(str2)) {
        return -1;
    }
    if (strcmp(str1, str2) != 0) {
        return -1;
    }
    return 0;
}
