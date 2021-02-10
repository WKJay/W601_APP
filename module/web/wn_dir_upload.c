#include <stdlib.h>
#include <string.h>

#include <rtthread.h>
#include <dfs_posix.h>

#include <webnet.h>
#include <wn_module.h>

#include "fal.h"
#include "web_utils.h"

#ifdef WEBNET_USING_UPLOAD

/**
 * upload manager
 */
typedef struct _upload_mgr {
    int fd;
    int total_file_size;
    int current_file_size;
    const char *base_path;
} upload_mgr_t;

static int upload_open(struct webnet_session *session) {
    upload_mgr_t *mgr = rt_malloc(sizeof(upload_mgr_t));
    if (mgr == NULL) {
        rt_kprintf("no memory for uplaod manager\r\n");
        return 0;
    }
    rt_memset(mgr, 0, sizeof(*mgr));
    mgr->current_file_size = 0;
    mgr->base_path = webnet_request_get_query(session->request, "path");
    return (int)mgr;
}

static int upload_close(struct webnet_session *session) {
    upload_mgr_t *mgr = (upload_mgr_t *)webnet_upload_get_userdata(session);
    if (mgr) rt_free(mgr);
    return 0;
}

static int upload_write(struct webnet_session *session, const void *data,
                        rt_size_t length) {
    const char *fname_p = NULL;
    static char old_filename[MAX_FILENAME_LENGTH];
    char filename[MAX_FILENAME_LENGTH];
    int filename_len = 0;
    upload_mgr_t *mgr = (upload_mgr_t *)webnet_upload_get_userdata(session);
    if (mgr == NULL) return 0;

    fname_p = webnet_upload_get_filename(session);
    filename_len = strlen(fname_p);
    if (filename_len > 128 - 1) {
        rt_kprintf("filename length too large\r\n");
        return 0;
    }

    if (strcmp(old_filename, fname_p) != 0) {  // new file
        if (mgr->fd > 0) close(mgr->fd);       // close current fd
        rt_strncpy(old_filename, fname_p, sizeof(old_filename));
        relative_path_2_absolute((char *)fname_p, filename, sizeof(filename),
                                 (char *)mgr->base_path);
        mgr->fd = create_file_by_path(filename);
    }

    if (mgr->fd > 0) {
        write(mgr->fd, data, length);
    }

    mgr->current_file_size += length;
    return length;
}

static int upload_done(struct webnet_session *session) {
    const char *mimetype;
    static char status[100];
    upload_mgr_t *mgr = (upload_mgr_t *)webnet_upload_get_userdata(session);
    close(mgr->fd);
    /* get mimetype */
    mimetype = mime_get_type(".html");

    /* set http header */
    session->request->result_code = 200;
    rt_memset(status, 0, sizeof(status));
    rt_snprintf(status, sizeof(status), "{\"code\":0,\"filesize\":%d}",
                mgr->current_file_size);

    webnet_session_set_header(session, mimetype, 200, "Ok", rt_strlen(status));
    webnet_session_printf(session, status);
    return 0;
}

const struct webnet_module_upload_entry upload_dir_upload = {
    "/upload/directory", upload_open, upload_close, upload_write, upload_done};

#endif /* WEBNET_USING_UPLOAD */
