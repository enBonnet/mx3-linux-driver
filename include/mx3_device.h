#ifndef MX3_DEVICE_H
#define MX3_DEVICE_H

#include "mx3_common.h"

int   device_find_mouse(const device_match_t *match);
char *device_name_from_fd(int fd);

#endif
