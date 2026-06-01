#ifndef MX3_COMMON_H
#define MX3_COMMON_H

#include <linux/input.h>
#include <linux/uinput.h>
#include <stdbool.h>
#include <time.h>

#define MX3_VERSION "1.0.0"

/* --------------------------------------------------------------------------
 * Log levels
 * -------------------------------------------------------------------------- */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
} log_level_t;

#define LOG_DEBUG(fmt, ...) mx3_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  mx3_log(LOG_INFO,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  mx3_log(LOG_WARN,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) mx3_log(LOG_ERROR, fmt, ##__VA_ARGS__)

void mx3_log(log_level_t level, const char *fmt, ...);
void mx3_log_set_level(log_level_t level);
void mx3_log_set_fp(void *fp);

/* --------------------------------------------------------------------------
 * Gesture direction
 * -------------------------------------------------------------------------- */
typedef enum {
    DIR_NONE = 0,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN,
    DIR_UPLEFT,
    DIR_UPRIGHT,
    DIR_DOWNLEFT,
    DIR_DOWNRIGHT,
    DIR_COUNT
} gesture_dir_t;

const char *gesture_dir_name(gesture_dir_t dir);

/* --------------------------------------------------------------------------
 * Button identifiers (index into config->buttons array)
 * -------------------------------------------------------------------------- */
typedef enum {
    BTN_IDX_FORWARD = 0,
    BTN_IDX_BACK,
    BTN_IDX_MIDDLE,
    BTN_IDX_EXTRA,
    BTN_IDX_SIDE,
    BTN_IDX_GESTURE,
    BTN_IDX_COUNT
} button_idx_t;

/* --------------------------------------------------------------------------
 * A single gesture action
 * -------------------------------------------------------------------------- */
typedef struct {
    gesture_dir_t direction;          /* DIR_NONE = tap */
    int           action_keys[8];     /* key codes to press, 0-terminated */
    int           action_key_count;
    int           press_duration_ms;  /* 0 = tap only, >0 = long press needed */
    char         *description;        /* human-readable */
} gesture_action_t;

/* --------------------------------------------------------------------------
 * Per-button configuration
 * -------------------------------------------------------------------------- */
typedef struct {
    int              event_code;      /* e.g. BTN_FORWARD */
    char            *name;            /* e.g. "forward" */
    int              motion_threshold;
    char            *gesture_key;     /* chord: which key to hold during gesture */
    gesture_action_t actions[DIR_COUNT + 1];  /* +1 for DIR_NONE (tap) */
    int              action_count;
} button_config_t;

/* --------------------------------------------------------------------------
 * Device match criteria
 * -------------------------------------------------------------------------- */
typedef struct {
    int  vid;          /* USB vendor ID (0 = any) */
    int  pid;          /* USB product ID (0 = any) */
    char name[128];    /* substring to match in device name */
} device_match_t;

/* --------------------------------------------------------------------------
 * Global configuration
 * -------------------------------------------------------------------------- */
typedef struct {
    device_match_t  device_match;
    button_config_t buttons[BTN_IDX_COUNT];
    int             button_count;
    double          tap_timeout;      /* seconds */
    int             motion_threshold; /* default, overridable per button */
    int             default_longpress_ms;
    log_level_t     log_level;
    char            pid_file[512];
    bool            daemonize;
} config_t;

/* --------------------------------------------------------------------------
 * MX3 USB vendor/product IDs
 * -------------------------------------------------------------------------- */
#define VID_LOGITECH 0x046d
#define PID_MX2       0x4082
#define PID_MX2S      0x4085
#define PID_MX3       0x4082  /* Some MX3 report same PID as MX2 */
#define PID_MX3_ALT   0xc08a
#define PID_MX3S      0x408a
#define PID_ANYWHERE3 0x4091

/* --------------------------------------------------------------------------
 * Configuration file paths (searched in order)
 * -------------------------------------------------------------------------- */
extern const char *CONFIG_SEARCH_PATHS[];
extern const int   CONFIG_SEARCH_PATHS_COUNT;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
config_t *config_new(void);
void      config_free(config_t *cfg);
int       config_load(config_t *cfg, const char *path);
int       config_load_default(config_t *cfg);
void      config_set_defaults(config_t *cfg);

#endif
