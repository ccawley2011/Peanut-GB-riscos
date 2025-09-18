#include "msgs.h"

#include <stdlib.h>

#include "oslib/messagetrans.h"

static messagetrans_control_block msgs_cb;
static void *msgs_buf = NULL;

os_error err_nomem = { 0, "nomem" };

os_error *msgs_open(const char *file) {
    os_error *err;
    int size;

    err = xmessagetrans_file_info(file, NULL, &size);
    if (err != NULL)
        return err;

    msgs_buf = malloc(size);
    if (!msgs_buf)
        return &err_nomem;

    err = xmessagetrans_open_file(&msgs_cb, file, msgs_buf);
    if (err != NULL) {
        free(msgs_buf);
        return err;
    }

    msgs_err_lookup(&err_nomem);
    return NULL;
}

void msgs_close(void) {
    messagetrans_close_file(&msgs_cb);
    free(msgs_buf);
}

const char *msgs_lookup(const char *token, int *used) {
    return messagetrans_lookup (&msgs_cb, token, NULL, 0, NULL, NULL, NULL, NULL, used);
}

os_error *msgs_err_lookup(os_error *err) {
    return xmessagetrans_error_lookup(err, &msgs_cb, err, sizeof(os_error), NULL, NULL, NULL, NULL);
}

os_error *msgs_err_lookup_1(os_error *err, const char *arg0) {
    return xmessagetrans_error_lookup(err, &msgs_cb, err, sizeof(os_error), arg0, NULL, NULL, NULL);
}

os_error *msgs_err_lookup_2(os_error *err, const char *arg0, const char *arg1) {
    return xmessagetrans_error_lookup(err, &msgs_cb, err, sizeof(os_error), arg0, arg1, NULL, NULL);
}

os_error *msgs_err_lookup_3(os_error *err, const char *arg0, const char *arg1, const char *arg2) {
    return xmessagetrans_error_lookup(err, &msgs_cb, err, sizeof(os_error), arg0, arg1, arg2, NULL);
}

os_error *msgs_err_lookup_4(os_error *err, const char *arg0, const char *arg1, const char *arg2, const char *arg3) {
    return xmessagetrans_error_lookup(err, &msgs_cb, err, sizeof(os_error), arg0, arg1, arg2, arg3);
}
