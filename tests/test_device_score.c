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

    int failed = 0;
    failed |= expect(exact == 175, "exact match score mismatch");
    failed |= expect(partial == 150, "partial name match score mismatch");
    failed |= expect(wrong_pid == 75, "wrong pid should only score name match");
    failed |= expect(wrong_name == 100, "wrong name should only score VID/PID match");
    failed |= expect(exact > partial && partial > wrong_pid, "score ordering mismatch");

    return failed ? 1 : 0;
}
