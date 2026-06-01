#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "mx3_common.h"

/* maximum number of registered keys */
#define MAX_REG_KEYS 64

static int g_registered_keys[MAX_REG_KEYS];
static int g_registered_key_count = 0;

static void remember_key(int code) {
    for (int i = 0; i < g_registered_key_count; i++) {
        if (g_registered_keys[i] == code) return;
    }
    if (g_registered_key_count < MAX_REG_KEYS) {
        g_registered_keys[g_registered_key_count++] = code;
    }
}

int uinput_create(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        LOG_ERROR("Cannot open /dev/uinput: %s", strerror(errno));
        LOG_ERROR("Try: sudo modprobe uinput && sudo usermod -aG input $USER");
        return -1;
    }

    /* enable key events */
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
        LOG_ERROR("Cannot set EV_KEY bit: %s", strerror(errno));
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
        LOG_ERROR("Cannot set EV_SYN bit: %s", strerror(errno));
        close(fd);
        return -1;
    }

    /* register a comprehensive set of common keys */
    int all_keys[] = {
        KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
        KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB,
        KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
        KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER,
        KEY_LEFTCTRL, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
        KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT, KEY_BACKSLASH,
        KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
        KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, KEY_KPASTERISK,
        KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK,
        KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
        KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
        KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20,
        KEY_F21, KEY_F22, KEY_F23, KEY_F24,
        KEY_NUMLOCK, KEY_SCROLLLOCK,
        KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4,
        KEY_KP5, KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9,
        KEY_KPMINUS, KEY_KPPLUS, KEY_KPDOT, KEY_KPENTER,
        KEY_LEFTMETA, KEY_RIGHTMETA, KEY_RIGHTALT, KEY_RIGHTCTRL,
        KEY_MUTE, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_PLAYPAUSE,
        KEY_STOPCD, KEY_PREVIOUSSONG, KEY_NEXTSONG,
        KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
        KEY_HOME, KEY_END, KEY_PAGEUP, KEY_PAGEDOWN,
        KEY_INSERT, KEY_DELETE,
        KEY_BRIGHTNESSDOWN, KEY_BRIGHTNESSUP,
        KEY_PRINT,
    };

    int nkeys = sizeof(all_keys) / sizeof(all_keys[0]);
    for (int i = 0; i < nkeys; i++) {
        if (ioctl(fd, UI_SET_KEYBIT, all_keys[i]) < 0) {
            LOG_WARN("Cannot register key 0x%x: %s", all_keys[i], strerror(errno));
        } else {
            remember_key(all_keys[i]);
        }
    }

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    strncpy(usetup.name, "MX3 Gesture Driver", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
        LOG_ERROR("Cannot setup uinput device: %s", strerror(errno));
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        LOG_ERROR("Cannot create uinput device: %s", strerror(errno));
        close(fd);
        return -1;
    }

    LOG_INFO("Virtual keyboard device created successfully");
    return fd;
}

void uinput_destroy(int fd) {
    if (fd < 0) return;
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    LOG_INFO("Virtual keyboard device destroyed");
}

void uinput_send_keys(int fd, const int keys[], int key_count) {
    if (fd < 0 || !keys || key_count <= 0 || key_count > 8) return;

    struct input_event ev;
    memset(&ev, 0, sizeof(ev));

    for (int i = 0; i < key_count; i++) {
        ev.type = EV_KEY;
        ev.code = keys[i];
        ev.value = 1;
        if (write(fd, &ev, sizeof(ev)) < 0) {
            LOG_ERROR("uinput write failed (press key 0x%x): %s", keys[i], strerror(errno));
            return;
        }
    }

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));

    usleep(10000);

    for (int i = key_count - 1; i >= 0; i--) {
        ev.type = EV_KEY;
        ev.code = keys[i];
        ev.value = 0;
        if (write(fd, &ev, sizeof(ev)) < 0) {
            LOG_ERROR("uinput write failed (release key 0x%x): %s", keys[i], strerror(errno));
            return;
        }
    }

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

void uinput_register_keys(int fd, const int keys[], int key_count) {
    for (int i = 0; i < key_count; i++) {
        if (ioctl(fd, UI_SET_KEYBIT, keys[i]) == 0) {
            remember_key(keys[i]);
        }
    }
}
