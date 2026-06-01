#ifndef MX3_UINPUT_H
#define MX3_UINPUT_H

#include "mx3_common.h"

int  uinput_create(void);
void uinput_destroy(int fd);
void uinput_send_keys(int fd, const int keys[], int key_count);
void uinput_register_keys(int fd, const int keys[], int key_count);

#endif
