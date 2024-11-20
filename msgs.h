#ifndef MSGS_H
#define MSGS_H

#include "oslib/os.h"

void msgs_open(const char *file);
void msgs_close(void);

const char *msgs_lookup(const char *token);

os_error *msgs_err_lookup(os_error *err);
os_error *msgs_err_lookup_1(os_error *err, const char *arg0);
os_error *msgs_err_lookup_2(os_error *err, const char *arg0, const char *arg1);
os_error *msgs_err_lookup_3(os_error *err, const char *arg0, const char *arg1, const char *arg2);
os_error *msgs_err_lookup_4(os_error *err, const char *arg0, const char *arg1, const char *arg2, const char *arg3);

#endif
