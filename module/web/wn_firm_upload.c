#include <stdlib.h>
#include <string.h>

#include <rtthread.h>
#include <dfs_posix.h>

#include <webnet.h>
#include <wn_module.h>

#include "fal.h"
#include "system.h"

#ifdef WEBNET_USING_UPLOAD

/**
 * upload file.
 */
static int file_size = 0;

static const char *get_file_name(struct webnet_session *session) {
    const char *path = RT_NULL, *path_last = RT_NULL;

    path_last = webnet_upload_get_filename(session);
    if (path_last == RT_NULL) {
        rt_kprintf("file name err!!\n");
        return RT_NULL;
    }

    path = strrchr(path_last, '\\');
    if (path != RT_NULL) {
        path++;
        path_last = path;
    }

    path = strrchr(path_last, '/');
    if (path != RT_NULL) {
        path++;
        path_last = path;
    }

    return path_last;
}

static int upload_open(struct webnet_session *session) {
    const struct fal_partition *part = RT_NULL;
    const char *file_name = RT_NULL;

    file_name = get_file_name(session);
    rt_kprintf("Upload FileName: %s\n", file_name);
    rt_kprintf("Content-Type   : %s\n", webnet_upload_get_content_type(session));

    if (webnet_upload_get_filename(session) != RT_NULL) {
        part = fal_partition_find("download");
        if (part == RT_NULL) {
            webnet_session_close(session);
            goto _exit;
        }
    }

    file_size = 0;

_exit:
    return (int)part;
}

static int upload_close(struct webnet_session *session) {
    rt_kprintf("Upload FileSize: %d\n", file_size);
    return 0;
}

static int upload_write(struct webnet_session *session, const void *data, rt_size_t length) {
    const struct fal_partition *part = RT_NULL;

    part = (const struct fal_partition *)webnet_upload_get_userdata(session);
    if (part == RT_NULL) return 0;

    rt_kprintf("write: length %d\n", length);
    fal_partition_write(part, file_size, data, length);

    file_size += length;

    return length;
}

static int upload_done(struct webnet_session *session) {
    const char *mimetype;
    static char status[100];

    /* get mimetype */
    mimetype = mime_get_type(".html");

    /* set http header */
    session->request->result_code = 200;

    rt_memset(status, 0, sizeof(status));
    rt_snprintf(status, sizeof(status), "{\"code\":0,\"filesize\":%d}", file_size);

    webnet_session_set_header(session, mimetype, 200, "Ok", rt_strlen(status));
    webnet_session_printf(session, status, file_size);

    system_reset();

    return 0;
}

const struct webnet_module_upload_entry upload_bin_upload = {
    "/upload/app", upload_open, upload_close, upload_write, upload_done};

#endif /* WEBNET_USING_UPLOAD */
