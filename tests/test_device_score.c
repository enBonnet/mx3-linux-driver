#include <stdio.h>
#include <string.h>

#include "mx3_device.h"

static int expect(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        return 1;
    }
    return 0;
}

int main(void) {
    device_match_t match;
    memset(&match, 0, sizeof(match));
    match.vid = 0x046d;
    match.pid = 0x408a;
    strcpy(match.name, "MX Master 3S");

    int exact = device_score_match(&match, 0x046d, 0x408a, "MX Master 3S");
    int partial = device_score_match(&match, 0x046d, 0x408a, "Logitech MX Master 3S");
    int wrong_pid = device_score_match(&match, 0x046d, 0x9999, "MX Master 3S");
    int wrong_name = device_score_match(&match, 0x046d, 0x408a, "Office Mouse");
    int wrong_vid = device_score_match(&match, 0x1234, 0x408a, "MX Master 3S");
    int bluetooth = device_score_match(&match, 0x0000, 0x0000, "MX Master 3S");
    int no_name = device_score_match(&match, 0x046d, 0x408a, NULL);
    int no_match = device_score_match(&match, 0x1234, 0x5678, "Office Mouse");

    /* Default substring "MX" must match both USB and Bluetooth evdev names. */
    device_match_t mx_match;
    memset(&mx_match, 0, sizeof(mx_match));
    mx_match.vid = VID_LOGITECH;
    mx_match.pid = 0;
    strcpy(mx_match.name, "MX");

    int usb_mx = device_score_match(&mx_match, VID_LOGITECH, 0x408a, "Logitech MX Master 3S");
    int bt_mx = device_score_match(&mx_match, 0x0000, 0x0000, "MX Master 3S");
    int bt_anywhere = device_score_match(&mx_match, 0x0000, 0x0000, "MX Anywhere 3");

    /* Any-VID/PID wildcard matching must not over-match non-matching names. */
    device_match_t any_match;
    memset(&any_match, 0, sizeof(any_match));

    int any_name = device_score_match(&any_match, 0x046d, 0x408a, "MX Master 3S");

    int failed = 0;
    failed |= expect(exact == 175, "exact match score mismatch");
    failed |= expect(partial == 150, "partial name match score mismatch");
    failed |= expect(wrong_pid == 75, "wrong pid should only score name match");
    failed |= expect(wrong_name == 100, "wrong name should only score VID/PID match");
    failed |= expect(wrong_vid == 75, "wrong vid should only score name match");
    failed |= expect(bluetooth == 75, "bluetooth name-only match score mismatch");
    failed |= expect(no_name == 100, "missing device name should only score VID/PID match");
    failed |= expect(no_match == 0, "non-matching VID/PID and name should score zero");
    failed |= expect(usb_mx == 150, "default 'MX' substring should match USB receiver name");
    failed |= expect(bt_mx == 50, "default 'MX' substring should match Bluetooth name");
    failed |= expect(bt_anywhere == 50, "default 'MX' substring should match Bluetooth Anywhere name");
    failed |= expect(any_name == 150, "empty vid/pid wildcard should match any device with a name");
    failed |= expect(exact > partial && partial > wrong_pid, "score ordering mismatch");

    return failed ? 1 : 0;
}
