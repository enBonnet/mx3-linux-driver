#include <stdio.h>

#include "mx3_gesture.h"

static int expect_dir(gesture_dir_t actual, gesture_dir_t expected, const char *name) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %d got %d\n", name, expected, actual);
        return 1;
    }
    return 0;
}

static int expect(int condition, const char *name) {
    if (!condition) {
        fprintf(stderr, "%s\n", name);
        return 1;
    }
    return 0;
}

int main(void) {
    gesture_tracker_t tracker;
    double duration = 0.0;
    int failed = 0;

    /* Inactive tracker must resolve to DIR_NONE. */
    gesture_tracker_init(&tracker, 50);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_NONE, "inactive tracker");
    failed |= expect_dir(gesture_tracker_is_active(&tracker), false, "is_active false after init");

    /* Tap: small motion below threshold. */
    gesture_tracker_init(&tracker, 50);
    gesture_tracker_start(&tracker, 0x115);
    failed |= expect_dir(gesture_tracker_is_active(&tracker), true, "is_active true after start");
    failed |= expect_dir(gesture_tracker_button(&tracker), 0x115, "button tracking");
    gesture_tracker_add_motion(&tracker, 10, 10);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, &duration), DIR_NONE, "tap threshold");
    failed |= expect(duration >= 0.0, "duration must be non-negative");
    gesture_tracker_stop(&tracker);
    failed |= expect_dir(gesture_tracker_is_active(&tracker), false, "is_active false after stop");

    /* Cardinal directions. */
    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 100, 5);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_RIGHT, "right swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x116);
    gesture_tracker_add_motion(&tracker, -120, 5);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_LEFT, "left swipe");
    failed |= expect_dir(gesture_tracker_button(&tracker), 0x116, "button tracking on back button");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, -5, -120);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_UP, "up swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 5, 120);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_DOWN, "down swipe");
    gesture_tracker_stop(&tracker);

    /* Diagonal directions. */
    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 100, 90);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_DOWNRIGHT, "downright swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, 100, -90);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_UPRIGHT, "upright swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, -100, 80);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_DOWNLEFT, "downleft swipe");
    gesture_tracker_stop(&tracker);

    gesture_tracker_start(&tracker, 0x115);
    gesture_tracker_add_motion(&tracker, -80, -100);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_UPLEFT, "upleft swipe");
    gesture_tracker_stop(&tracker);

    /* Motion added after stop must be ignored. */
    gesture_tracker_stop(&tracker);
    gesture_tracker_add_motion(&tracker, 1000, 0);
    failed |= expect_dir(gesture_tracker_resolve(&tracker, NULL), DIR_NONE, "motion after stop ignored");

    return failed ? 1 : 0;
}
