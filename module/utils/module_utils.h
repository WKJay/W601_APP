#ifndef __UTILS_H
#define __UTILS_H

//读取文件数据，需注意使用后需要释放空间
unsigned char *file_read(char *path);
//获取剩余存储空间（单位：KB）
long long get_disk_free(void);
//检查剩余存储空间容量，若过小则会格式化
int check_disk_free(void);
// 检测文件夹的存在，若不存在则创建
int check_create_path(const char *path);
//比较两字符串是否相等
int str_compare(char *str1, char *str2);
// free前检查指针是否为空
void safe_free(void *ptr);

#endif /* __UTILS_H */
