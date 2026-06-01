#ifndef MX3_GESTURE_H
#define MX3_GESTURE_H

#include "mx3_common.h"

typedef struct {
    bool          gesture_active;
    int           button_code;       /* which button triggered it */
    struct timespec press_time;

    int           accum_x;
    int           accum_y;

    int           threshold;

    /* chord support: another key held during gesture */
    int           chord_key_code;
    bool          chord_key_held;
} gesture_tracker_t;

void           gesture_tracker_init(gesture_tracker_t *gt, int threshold);
void           gesture_tracker_start(gesture_tracker_t *gt, int button_code);
void           gesture_tracker_stop(gesture_tracker_t *gt);
void           gesture_tracker_add_motion(gesture_tracker_t *gt, int dx, int dy);
gesture_dir_t  gesture_tracker_resolve(gesture_tracker_t *gt, double *duration_out);
bool           gesture_tracker_is_active(const gesture_tracker_t *gt);
int            gesture_tracker_button(const gesture_tracker_t *gt);

#endif
