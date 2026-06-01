#include <stdio.h>

#include "mx3_gesture.h"

static int expect_dir(gesture_dir_t actual, gesture_dir_t expected, const char *name) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %d got %d\n", name, expected, actual);
        return 1;
    }
    return 0;
}

int main(void) {
    gesture_tracker_t tracker;
    double duration = 0.0;
    int failed = 0;

    gesture_tracker_init(&tracker, 50);
    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 10, 10);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, &duration), DIR_NONE, "tap threshold");
    failed |= expect_dir(gesture_tracker_button(&tracker), 0x115, "button tracking");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 100, 5);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_RIGHT, "right swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, -5, -120);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_UP, "up swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 100, -90);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_UPRIGHT, "upright swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, -100, 80);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_DOWNLEFT, "downleft swipe");
    gesture_tracker_stop(&tracker);

    return failed ? 1 : 0;
}
