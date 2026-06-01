#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mx3_common.h"

static int expect(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        return 1;
    }
    return 0;
}

int main(void) {
    const char *path = "/tmp/mx3-test-config.conf";
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    fputs("[general]\n"
          "tap_timeout = 0.4\n"
          "motion_threshold = 75\n"
          "log_level = debug\n"
          "\n"
          "[device]\n"
          "vid = 0x046d\n"
          "pid = 0x408a\n"
          "device_name = MX Master 3S\n"
          "\n"
          "[button.forward]\n"
          "event_code = 0x115\n"
          "motion_threshold = 60\n"
          "action.left.keys = KEY_LEFTCTRL, KEY_PAGEUP\n"
          "action.left.duration = 150\n"
          "action.left.description = Previous tab\n"
          "action.upright.keys = KEY_LEFTCTRL, KEY_RIGHT\n"
          "action.upright.description = Up right\n"
          "action.unknown.keys = KEY_DOES_NOT_EXIST\n",
          fp);
    fclose(fp);

    config_t *cfg = config_new();
    if (!cfg) {
        unlink(path);
        fprintf(stderr, "config_new failed\n");
        return 1;
    }

    if (config_load(cfg, path) != 0) {
        config_free(cfg);
        unlink(path);
        fprintf(stderr, "config_load failed\n");
        return 1;
    }

    int failed = 0;
    failed |= expect(cfg->tap_timeout == 0.4, "tap_timeout was not loaded");
    failed |= expect(cfg->motion_threshold == 75, "motion_threshold was not loaded");
    failed |= expect(cfg->log_level == LOG_DEBUG, "log_level was not loaded");
    failed |= expect(cfg->device_match.pid == 0x408a, "device pid was not loaded");
    failed |= expect(strcmp(cfg->device_match.name, "MX Master 3S") == 0, "device name was not loaded");
    failed |= expect(cfg->buttons[BTN_IDX_FORWARD].motion_threshold == 60, "button motion threshold was not overridden");

    gesture_action_t *left = config_find_action(cfg, BTN_IDX_FORWARD, DIR_LEFT);
    failed |= expect(left != NULL, "left action missing");
    if (left) {
        failed |= expect(left->action_key_count == 2, "left action key count mismatch");
        failed |= expect(left->action_keys[0] == KEY_LEFTCTRL, "left action first key mismatch");
        failed |= expect(left->action_keys[1] == KEY_PAGEUP, "left action second key mismatch");
        failed |= expect(left->press_duration_ms == 150, "left action duration mismatch");
        failed |= expect(strcmp(left->description, "Previous tab") == 0, "left action description mismatch");
    }

    gesture_action_t *upright = config_find_action(cfg, BTN_IDX_FORWARD, DIR_UPRIGHT);
    failed |= expect(upright != NULL, "upright action missing");
    if (upright) {
        failed |= expect(upright->action_key_count == 2, "upright action key count mismatch");
    }

    config_free(cfg);
    unlink(path);
    return failed ? 1 : 0;
}
