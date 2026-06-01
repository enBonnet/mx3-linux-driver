#ifndef MX3_DEVICE_H
#define MX3_DEVICE_H

#include "mx3_common.h"

int   device_find_mouse(const device_match_t *match);
int   device_score_match(const device_match_t *match, int vid, int pid, const char *device_name);
char *device_name_from_fd(int fd);

#endif
