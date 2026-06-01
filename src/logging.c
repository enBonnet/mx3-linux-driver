#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "mx3_common.h"

static log_level_t g_log_level = LOG_INFO;
static FILE       *g_log_fp    = NULL;

static const char *level_prefix(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "????";
    }
}

void mx3_log_set_level(log_level_t level) {
    g_log_level = level;
}

void mx3_log_set_fp(void *fp) {
    g_log_fp = (FILE *)fp;
}

void mx3_log(log_level_t level, const char *fmt, ...) {
    if (level < g_log_level) return;

    FILE *out = g_log_fp ? g_log_fp : stderr;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm lt;
    localtime_r(&tv.tv_sec, &lt);

    fprintf(out, "[%02d:%02d:%02d.%03ld] %-5s ",
            lt.tm_hour, lt.tm_min, lt.tm_sec,
            tv.tv_usec / 1000,
            level_prefix(level));

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
    fflush(out);
}

const char *gesture_dir_name(gesture_dir_t dir) {
    switch (dir) {
        case DIR_NONE:       return "tap";
        case DIR_LEFT:       return "left";
        case DIR_RIGHT:      return "right";
        case DIR_UP:         return "up";
        case DIR_DOWN:       return "down";
        case DIR_UPLEFT:     return "upleft";
        case DIR_UPRIGHT:    return "upright";
        case DIR_DOWNLEFT:   return "downleft";
        case DIR_DOWNRIGHT:  return "downright";
        default:             return "unknown";
    }
}
