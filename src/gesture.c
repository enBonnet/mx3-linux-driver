#include <stdlib.h>
#include <string.h>

#include "mx3_common.h"
#include "mx3_gesture.h"

void gesture_tracker_init(gesture_tracker_t *gt, int threshold) {
    memset(gt, 0, sizeof(*gt));
    gt->threshold = threshold;
}

void gesture_tracker_start(gesture_tracker_t *gt, int button_code) {
    gt->gesture_active = true;
    gt->button_code = button_code;
    gt->accum_x = 0;
    gt->accum_y = 0;
    clock_gettime(CLOCK_MONOTONIC, &gt->press_time);
    LOG_DEBUG("Gesture tracker started for button 0x%x, threshold=%d",
              button_code, gt->threshold);
}

void gesture_tracker_stop(gesture_tracker_t *gt) {
    gt->gesture_active = false;
    gt->button_code = 0;
    gt->accum_x = 0;
    gt->accum_y = 0;
}

void gesture_tracker_add_motion(gesture_tracker_t *gt, int dx, int dy) {
    if (!gt->gesture_active) return;
    gt->accum_x += dx;
    gt->accum_y += dy;
}

gesture_dir_t gesture_tracker_resolve(gesture_tracker_t *gt, double *duration_out) {
    if (!gt->gesture_active) return DIR_NONE;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (duration_out) {
        *duration_out = (now.tv_sec - gt->press_time.tv_sec) +
                        (now.tv_nsec - gt->press_time.tv_nsec) / 1e9;
    }

    int ax = abs(gt->accum_x);
    int ay = abs(gt->accum_y);
    int max_axis = (ax > ay) ? ax : ay;

    if (max_axis < gt->threshold) return DIR_NONE;

    /* diagonal if both axes exceed half the maximum */
    int half_max = max_axis / 2;
    if (ax > half_max && ay > half_max) {
        if (gt->accum_x > 0 && gt->accum_y > 0)   return DIR_DOWNRIGHT;
        if (gt->accum_x > 0 && gt->accum_y < 0)   return DIR_UPRIGHT;
        if (gt->accum_x < 0 && gt->accum_y > 0)   return DIR_DOWNLEFT;
        if (gt->accum_x < 0 && gt->accum_y < 0)   return DIR_UPLEFT;
    }

    /* pure axes */
    if (ax > ay) {
        return (gt->accum_x > 0) ? DIR_RIGHT : DIR_LEFT;
    } else {
        return (gt->accum_y > 0) ? DIR_DOWN : DIR_UP;
    }
}

bool gesture_tracker_is_active(const gesture_tracker_t *gt) {
    return gt->gesture_active;
}

int gesture_tracker_button(const gesture_tracker_t *gt) {
    return gt->button_code;
}
