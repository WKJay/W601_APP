#ifndef __WEB_UTILS_H
#define __WEB_UTILS_H
#define MAX_FILENAME_LENGTH 128

int relative_path_2_absolute(char *path_from, char *path_to, int to_length,
                             char *absolute_prefix);
int create_file_by_path(char *path);
int web_file_init(void);

#endif /* __WEB_UTILS_H */
