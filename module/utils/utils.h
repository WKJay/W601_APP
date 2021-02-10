#ifndef __UTILS_H
#define __UTILS_H

//获取剩余存储空间（单位：KB）
long long get_disk_free(void);
//检查剩余存储空间容量，若过小则会格式化
int check_disk_free(void);
// 检测文件夹的存在，若不存在则创建
int check_create_path(const char *path);

#endif /* __UTILS_H */
