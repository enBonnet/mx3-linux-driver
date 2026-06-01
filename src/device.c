#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <linux/input.h>

#include "mx3_common.h"

#define MAX_PATH 512

static int check_device_usb(int fd, int vid, int pid) {
    if (vid == 0 && pid == 0) return 1;  /* match any */

    struct input_id id;
    if (ioctl(fd, EVIOCGID, &id) < 0) {
        LOG_DEBUG("Cannot get device ID — skipping VID/PID check");
        return 0;
    }

    if ((vid == 0 || id.vendor == vid) && (pid == 0 || id.product == pid)) {
        LOG_DEBUG("Device matches VID:PID %04x:%04x", id.vendor, id.product);
        return 1;
    }
    return 0;
}

static int check_device_name(int fd, const char *substr) {
    if (!substr || !*substr) return 1;  /* match any */

    char name[256] = {0};
    if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) < 0) {
        LOG_DEBUG("Cannot get device name");
        return 0;
    }

    if (strcasestr(name, substr)) {
        LOG_DEBUG("Device name '%s' matches '%s'", name, substr);
        return 1;
    }
    return 0;
}

static int check_device_caps(int fd) {
    unsigned long evbit = 0;
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) < 0) return 0;

    /* must have EV_KEY and EV_REL */
    if (!(evbit & (1 << EV_KEY)))  return 0;
    if (!(evbit & (1 << EV_REL)))  return 0;

    /* must have mouse buttons */
    unsigned long keybit[KEY_CNT / (sizeof(unsigned long) * 8) + 1];
    memset(keybit, 0, sizeof(keybit));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) return 0;

    int has_button = 0;
    if (keybit[BTN_LEFT / (sizeof(unsigned long) * 8)] &
        (1UL << (BTN_LEFT % (sizeof(unsigned long) * 8)))) has_button = 1;
    if (keybit[BTN_MOUSE / (sizeof(unsigned long) * 8)] &
        (1UL << (BTN_MOUSE % (sizeof(unsigned long) * 8)))) has_button = 1;

    return has_button;
}

int device_score_match(const device_match_t *match, int vid, int pid, const char *device_name) {
    int score = 0;

    if ((match->vid == 0 && match->pid == 0) ||
        ((match->vid == 0 || vid == match->vid) &&
         (match->pid == 0 || pid == match->pid))) {
        score += 100;
    }

    if (!match->name[0]) {
        score += 50;
    } else if (device_name && strcasestr(device_name, match->name)) {
        score += 50;
        if (strcmp(device_name, match->name) == 0) score += 25;
    }

    return score;
}

int device_find_mouse(const device_match_t *match) {
    DIR *dir = opendir("/dev/input");
    if (!dir) {
        LOG_ERROR("Cannot open /dev/input: %s", strerror(errno));
        return -1;
    }

    struct dirent *entry;
    char path[MAX_PATH];
    int best_fd = -1;
    int best_score = 0;

    LOG_INFO("Scanning /dev/input for mouse device (VID:%04x PID:%04x name:'%s')",
             match->vid, match->pid, match->name);

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            LOG_DEBUG("Cannot open %s: %s", path, strerror(errno));
            continue;
        }

        if (!check_device_caps(fd)) {
            close(fd);
            continue;
        }

        int score = 0;
        if (check_device_usb(fd, match->vid, match->pid) && check_device_name(fd, match->name)) {
            char name[256] = {0};
            struct input_id id = {0};

            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0 &&
                ioctl(fd, EVIOCGID, &id) >= 0) {
                score = device_score_match(match, id.vendor, id.product, name);
            }
        } else {
            if (check_device_usb(fd, match->vid, match->pid)) score += 100;
            if (check_device_name(fd, match->name)) score += 50;
        }

        if (score > best_score) {
            if (best_fd >= 0) close(best_fd);
            best_fd = fd;
            best_score = score;

            char name[256] = {0};
            ioctl(best_fd, EVIOCGNAME(sizeof(name) - 1), name);
            struct input_id id;
            ioctl(best_fd, EVIOCGID, &id);
            LOG_INFO("  Best match so far: %s (VID:%04x PID:%04x name:'%s') score=%d",
                     path, id.vendor, id.product, name, score);
        } else {
            close(fd);
        }
    }

    closedir(dir);

    if (best_fd < 0) {
        LOG_ERROR("No matching mouse device found");
        return -1;
    }

    char name[256] = {0};
    ioctl(best_fd, EVIOCGNAME(sizeof(name) - 1), name);
    struct input_id id;
    ioctl(best_fd, EVIOCGID, &id);
    LOG_INFO("Selected device: VID:%04x PID:%04x name:'%s'", id.vendor, id.product, name);
    return best_fd;
}

char *device_name_from_fd(int fd) {
    static char buf[256];
    memset(buf, 0, sizeof(buf));
    ioctl(fd, EVIOCGNAME(sizeof(buf) - 1), buf);
    return buf;
}
