#include "msgs.h"

#include "oslib/messagetrans.h"

static messagetrans_control_block msgs_cb;

void msgs_open(const char *file) {
    messagetrans_open_file(&msgs_cb, file, NULL);
}

void msgs_close(void) {
    messagetrans_close_file(&msgs_cb);
}

const char *msgs_lookup(const char *token) {
    return messagetrans_lookup (&msgs_cb, token, NULL, 0, NULL, NULL, NULL, NULL, NULL);
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
