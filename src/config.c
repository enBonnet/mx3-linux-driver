#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "mx3_common.h"

/* --------------------------------------------------------------------------
 * Config search paths (resolved at runtime with $HOME)
 * -------------------------------------------------------------------------- */
static const char *SEARCH_TEMPLATES[] = {
    "./mx3.conf",
    "%s/.config/mx3/config.conf",
    "%s/.config/mx3.conf",
    "/etc/mx3/config.conf",
    NULL
};

/* --------------------------------------------------------------------------
 * Key name → linux/input.h key code mapping
 * -------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    int         code;
} key_map_t;

static key_map_t key_map[] = {
    {"KEY_ESC",             KEY_ESC},
    {"KEY_1",               KEY_1},          {"KEY_2",               KEY_2},
    {"KEY_3",               KEY_3},          {"KEY_4",               KEY_4},
    {"KEY_5",               KEY_5},          {"KEY_6",               KEY_6},
    {"KEY_7",               KEY_7},          {"KEY_8",               KEY_8},
    {"KEY_9",               KEY_9},          {"KEY_0",               KEY_0},
    {"KEY_MINUS",           KEY_MINUS},      {"KEY_EQUAL",           KEY_EQUAL},
    {"KEY_BACKSPACE",       KEY_BACKSPACE},  {"KEY_TAB",             KEY_TAB},
    {"KEY_Q",               KEY_Q},          {"KEY_W",               KEY_W},
    {"KEY_E",               KEY_E},          {"KEY_R",               KEY_R},
    {"KEY_T",               KEY_T},          {"KEY_Y",               KEY_Y},
    {"KEY_U",               KEY_U},          {"KEY_I",               KEY_I},
    {"KEY_O",               KEY_O},          {"KEY_P",               KEY_P},
    {"KEY_LEFTBRACE",       KEY_LEFTBRACE},  {"KEY_RIGHTBRACE",      KEY_RIGHTBRACE},
    {"KEY_ENTER",           KEY_ENTER},      {"KEY_LEFTCTRL",        KEY_LEFTCTRL},
    {"KEY_A",               KEY_A},          {"KEY_S",               KEY_S},
    {"KEY_D",               KEY_D},          {"KEY_F",               KEY_F},
    {"KEY_G",               KEY_G},          {"KEY_H",               KEY_H},
    {"KEY_J",               KEY_J},          {"KEY_K",               KEY_K},
    {"KEY_L",               KEY_L},          {"KEY_SEMICOLON",       KEY_SEMICOLON},
    {"KEY_APOSTROPHE",      KEY_APOSTROPHE}, {"KEY_GRAVE",           KEY_GRAVE},
    {"KEY_LEFTSHIFT",       KEY_LEFTSHIFT},  {"KEY_BACKSLASH",       KEY_BACKSLASH},
    {"KEY_Z",               KEY_Z},          {"KEY_X",               KEY_X},
    {"KEY_C",               KEY_C},          {"KEY_V",               KEY_V},
    {"KEY_B",               KEY_B},          {"KEY_N",               KEY_N},
    {"KEY_M",               KEY_M},          {"KEY_COMMA",           KEY_COMMA},
    {"KEY_DOT",             KEY_DOT},        {"KEY_SLASH",           KEY_SLASH},
    {"KEY_RIGHTSHIFT",      KEY_RIGHTSHIFT}, {"KEY_KPASTERISK",      KEY_KPASTERISK},
    {"KEY_LEFTALT",         KEY_LEFTALT},    {"KEY_SPACE",           KEY_SPACE},
    {"KEY_CAPSLOCK",        KEY_CAPSLOCK},   {"KEY_F1",              KEY_F1},
    {"KEY_F2",              KEY_F2},         {"KEY_F3",              KEY_F3},
    {"KEY_F4",              KEY_F4},         {"KEY_F5",              KEY_F5},
    {"KEY_F6",              KEY_F6},         {"KEY_F7",              KEY_F7},
    {"KEY_F8",              KEY_F8},         {"KEY_F9",              KEY_F9},
    {"KEY_F10",             KEY_F10},        {"KEY_F11",             KEY_F11},
    {"KEY_F12",             KEY_F12},        {"KEY_F13",             KEY_F13},
    {"KEY_F14",             KEY_F14},        {"KEY_F15",             KEY_F15},
    {"KEY_F16",             KEY_F16},        {"KEY_F17",             KEY_F17},
    {"KEY_F18",             KEY_F18},        {"KEY_F19",             KEY_F19},
    {"KEY_F20",             KEY_F20},        {"KEY_F21",             KEY_F21},
    {"KEY_F22",             KEY_F22},        {"KEY_F23",             KEY_F23},
    {"KEY_F24",             KEY_F24},
    {"KEY_NUMLOCK",         KEY_NUMLOCK},    {"KEY_SCROLLLOCK",      KEY_SCROLLLOCK},
    {"KEY_KP0",             KEY_KP0},        {"KEY_KP1",             KEY_KP1},
    {"KEY_KP2",             KEY_KP2},        {"KEY_KP3",             KEY_KP3},
    {"KEY_KP4",             KEY_KP4},        {"KEY_KP5",             KEY_KP5},
    {"KEY_KP6",             KEY_KP6},        {"KEY_KP7",             KEY_KP7},
    {"KEY_KP8",             KEY_KP8},        {"KEY_KP9",             KEY_KP9},
    {"KEY_KPMINUS",         KEY_KPMINUS},    {"KEY_KPPLUS",          KEY_KPPLUS},
    {"KEY_KPDOT",           KEY_KPDOT},      {"KEY_KPENTER",         KEY_KPENTER},
    {"KEY_LEFTMETA",        KEY_LEFTMETA},   {"KEY_RIGHTMETA",       KEY_RIGHTMETA},
    {"KEY_RIGHTALT",        KEY_RIGHTALT},   {"KEY_RIGHTCTRL",       KEY_RIGHTCTRL},
    {"KEY_MUTE",            KEY_MUTE},       {"KEY_VOLUMEDOWN",      KEY_VOLUMEDOWN},
    {"KEY_VOLUMEUP",        KEY_VOLUMEUP},   {"KEY_PLAYPAUSE",       KEY_PLAYPAUSE},
    {"KEY_STOPCD",          KEY_STOPCD},     {"KEY_PREVIOUSSONG",    KEY_PREVIOUSSONG},
    {"KEY_NEXTSONG",        KEY_NEXTSONG},   {"KEY_LEFT",            KEY_LEFT},
    {"KEY_RIGHT",           KEY_RIGHT},      {"KEY_UP",              KEY_UP},
    {"KEY_DOWN",            KEY_DOWN},       {"KEY_HOME",            KEY_HOME},
    {"KEY_END",             KEY_END},        {"KEY_PAGEUP",          KEY_PAGEUP},
    {"KEY_PAGEDOWN",        KEY_PAGEDOWN},   {"KEY_INSERT",          KEY_INSERT},
    {"KEY_DELETE",          KEY_DELETE},     {"KEY_BRIGHTNESSDOWN",  KEY_BRIGHTNESSDOWN},
    {"KEY_BRIGHTNESSUP",    KEY_BRIGHTNESSUP}, {"KEY_PRINT",         KEY_PRINT},
    {NULL, 0}
};

static int lookup_key(const char *name) {
    for (int i = 0; key_map[i].name; i++) {
        if (strcasecmp(key_map[i].name, name) == 0) return key_map[i].code;
    }
    return -1;
}

/* --------------------------------------------------------------------------
 * Direction name → enum
 * -------------------------------------------------------------------------- */
static gesture_dir_t parse_direction(const char *s) {
    if (!s) return DIR_NONE;
    if (strcasecmp(s, "tap") == 0)        return DIR_NONE;
    if (strcasecmp(s, "left") == 0)       return DIR_LEFT;
    if (strcasecmp(s, "right") == 0)      return DIR_RIGHT;
    if (strcasecmp(s, "up") == 0)         return DIR_UP;
    if (strcasecmp(s, "down") == 0)       return DIR_DOWN;
    if (strcasecmp(s, "upleft") == 0)     return DIR_UPLEFT;
    if (strcasecmp(s, "upright") == 0)    return DIR_UPRIGHT;
    if (strcasecmp(s, "downleft") == 0)   return DIR_DOWNLEFT;
    if (strcasecmp(s, "downright") == 0)  return DIR_DOWNRIGHT;
    return DIR_NONE;
}

/* --------------------------------------------------------------------------
 * Trivial INI parser — reads [section] and key = value pairs
 * -------------------------------------------------------------------------- */
static char *strip(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

#define MAX_LINE 512

int config_load(config_t *cfg, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        LOG_DEBUG("Config file '%s' not found: %s", path, strerror(errno));
        return -1;
    }

    LOG_INFO("Loading configuration from '%s'", path);

    char line[MAX_LINE];
    button_config_t *cur_button = NULL;
    gesture_action_t *cur_action = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *s = strip(line);

        /* skip blanks and comments */
        if (!*s || *s == '#' || *s == ';') continue;

        /* section header */
        if (s[0] == '[') {
            char *end = strchr(s, ']');
            if (!end) continue;
            *end = '\0';
            s = strip(s + 1);

            if (strcasecmp(s, "general") == 0) {
                cur_button = NULL;
                cur_action = NULL;
            } else if (strcasecmp(s, "device") == 0) {
                cur_button = NULL;
                cur_action = NULL;
            } else {
                /* look for [button.xxx] */
                const char *bp = s;
                const char *btn_prefix = "button.";
                if (strncasecmp(bp, btn_prefix, strlen(btn_prefix)) == 0) {
                    bp += strlen(btn_prefix);
                    int found = -1;
                    for (int i = 0; i < BTN_IDX_COUNT; i++) {
                        if (cfg->buttons[i].name &&
                            strcasecmp(cfg->buttons[i].name, bp) == 0) {
                            found = i;
                            break;
                        }
                    }
                    if (found >= 0) {
                        cur_button = &cfg->buttons[found];
                        cur_action = NULL;
                    } else {
                        LOG_WARN("Unknown button section '[%s]'", s);
                        cur_button = NULL;
                    }
                } else {
                    LOG_WARN("Unknown section '[%s]'", s);
                    cur_button = NULL;
                }
                cur_action = NULL;
            }
            continue;
        }

        /* key = value */
        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = strip(s);
        char *val = strip(eq + 1);

        if (!cur_button) {
            /* general or device */
            if (strcasecmp(key, "tap_timeout") == 0) {
                cfg->tap_timeout = atof(val);
            } else if (strcasecmp(key, "motion_threshold") == 0) {
                cfg->motion_threshold = atoi(val);
            } else if (strcasecmp(key, "default_longpress_ms") == 0) {
                cfg->default_longpress_ms = atoi(val);
            } else if (strcasecmp(key, "log_level") == 0) {
                if (strcasecmp(val, "debug") == 0) cfg->log_level = LOG_DEBUG;
                else if (strcasecmp(val, "info") == 0) cfg->log_level = LOG_INFO;
                else if (strcasecmp(val, "warn") == 0) cfg->log_level = LOG_WARN;
                else if (strcasecmp(val, "error") == 0) cfg->log_level = LOG_ERROR;
            } else if (strcasecmp(key, "vid") == 0) {
                cfg->device_match.vid = strtol(val, NULL, 0);
            } else if (strcasecmp(key, "pid") == 0) {
                cfg->device_match.pid = strtol(val, NULL, 0);
            } else if (strcasecmp(key, "device_name") == 0) {
                strncpy(cfg->device_match.name, val, sizeof(cfg->device_match.name) - 1);
            }
            continue;
        }

        /* button section */
        if (strcasecmp(key, "event_code") == 0) {
            cur_button->event_code = strtol(val, NULL, 0);
        } else if (strcasecmp(key, "motion_threshold") == 0) {
            cur_button->motion_threshold = atoi(val);
        } else if (strcasecmp(key, "chord") == 0) {
            /* store chord key name */
            cur_button->gesture_key = strdup(val);
        } else if (strncasecmp(key, "action.", 7) == 0) {
            char action_part[128];
            strncpy(action_part, key + 7, sizeof(action_part) - 1);
            action_part[sizeof(action_part) - 1] = '\0';

            /* find or create action slot */
            cur_action = NULL;
            for (int i = 0; i < cur_button->action_count; i++) {
                if (gesture_dir_name(cur_button->actions[i].direction) == NULL) continue;
                /* reuse existing */
            }

            /* parse field: action.direction.keys, action.direction.duration, action.direction.description */
            char *dot = strchr(action_part, '.');
            char dir_name[64] = {0};
            if (dot) {
                size_t len = dot - action_part;
                if (len >= sizeof(dir_name)) len = sizeof(dir_name) - 1;
                memcpy(dir_name, action_part, len);
                dir_name[len] = '\0';
                char *field = dot + 1;

                gesture_dir_t dir = parse_direction(dir_name);

                /* find or create action for this direction */
                int idx = -1;
                for (int i = 0; i < cur_button->action_count; i++) {
                    if (cur_button->actions[i].direction == dir) {
                        idx = i;
                        break;
                    }
                }
                if (idx < 0) {
                    idx = cur_button->action_count++;
                    memset(&cur_button->actions[idx], 0, sizeof(gesture_action_t));
                    cur_button->actions[idx].direction = dir;
                }
                cur_action = &cur_button->actions[idx];

                if (strcasecmp(field, "keys") == 0) {
                    /* comma-separated key names */
                    cur_action->action_key_count = 0;
                    char *tk = strtok(val, ",");
                    while (tk && cur_action->action_key_count < 8) {
                        int k = lookup_key(strip(tk));
                        if (k >= 0) {
                            cur_action->action_keys[cur_action->action_key_count++] = k;
                        } else {
                            LOG_WARN("Unknown key '%s' in config", strip(tk));
                        }
                        tk = strtok(NULL, ",");
                    }
                } else if (strcasecmp(field, "duration") == 0) {
                    cur_action->press_duration_ms = atoi(val);
                } else if (strcasecmp(field, "description") == 0) {
                    cur_action->description = strdup(val);
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

/* --------------------------------------------------------------------------
 * Set sensible defaults for all buttons
 * -------------------------------------------------------------------------- */
void config_set_defaults(config_t *cfg) {
    cfg->tap_timeout = 0.2;
    cfg->motion_threshold = 50;
    cfg->default_longpress_ms = 500;
    cfg->log_level = LOG_INFO;
    cfg->pid_file[0] = '\0';
    cfg->daemonize = false;

    /* device match defaults for Logitech MX3 */
    cfg->device_match.vid = VID_LOGITECH;
    cfg->device_match.pid = 0;  /* match any Logitech product */
    strcpy(cfg->device_match.name, "Logitech");

    /* button definitions */
    static const struct {
        button_idx_t idx;
        int          code;
        const char  *name;
    } btn_defs[] = {
        { BTN_IDX_FORWARD, BTN_FORWARD,   "forward" },
        { BTN_IDX_BACK,    BTN_BACK,      "back"    },
        { BTN_IDX_MIDDLE,  BTN_MIDDLE,    "middle"  },
        { BTN_IDX_EXTRA,   BTN_EXTRA,     "extra"   },
        { BTN_IDX_SIDE,    BTN_SIDE,      "side"    },
        { BTN_IDX_GESTURE, 0x11a,         "gesture" }, /* BTN_TASK on some MX3 models */
    };

    for (size_t i = 0; i < sizeof(btn_defs)/sizeof(btn_defs[0]); i++) {
        button_config_t *b = &cfg->buttons[btn_defs[i].idx];
        b->event_code = btn_defs[i].code;
        b->name = strdup(btn_defs[i].name);
        b->motion_threshold = cfg->motion_threshold;
        b->gesture_key = NULL;
        b->action_count = 0;
        memset(b->actions, 0, sizeof(b->actions));
    }
    cfg->button_count = BTN_IDX_COUNT;

    /* Default gestures for forward button */
    button_config_t *fw = &cfg->buttons[BTN_IDX_FORWARD];
    int i = 0;

    fw->actions[i].direction = DIR_NONE;
    fw->actions[i].action_keys[0] = KEY_LEFTMETA;
    fw->actions[i].action_key_count = 1;
    fw->actions[i].description = strdup("Tap: Open app launcher (Super)");
    i++;

    fw->actions[i].direction = DIR_LEFT;
    fw->actions[i].action_keys[0] = KEY_LEFTMETA;
    fw->actions[i].action_keys[1] = KEY_LEFTBRACE;
    fw->actions[i].action_key_count = 2;
    fw->actions[i].description = strdup("Left gesture: Previous workspace (Super+[)");
    i++;

    fw->actions[i].direction = DIR_RIGHT;
    fw->actions[i].action_keys[0] = KEY_LEFTMETA;
    fw->actions[i].action_keys[1] = KEY_RIGHTBRACE;
    fw->actions[i].action_key_count = 2;
    fw->actions[i].description = strdup("Right gesture: Next workspace (Super+])");
    i++;

    fw->actions[i].direction = DIR_UP;
    fw->actions[i].action_keys[0] = KEY_VOLUMEUP;
    fw->actions[i].action_key_count = 1;
    fw->actions[i].description = strdup("Up gesture: Volume up");
    i++;

    fw->actions[i].direction = DIR_DOWN;
    fw->actions[i].action_keys[0] = KEY_VOLUMEDOWN;
    fw->actions[i].action_key_count = 1;
    fw->actions[i].description = strdup("Down gesture: Volume down");
    i++;
    fw->action_count = i;

    /* Default gestures for back button */
    button_config_t *bk = &cfg->buttons[BTN_IDX_BACK];
    i = 0;
    bk->actions[i].direction = DIR_NONE;
    bk->actions[i].action_keys[0] = KEY_LEFTMETA;
    bk->actions[i].action_key_count = 1;
    bk->actions[i].description = strdup("Tap: Super");
    i++;

    bk->actions[i].direction = DIR_LEFT;
    bk->actions[i].action_keys[0] = KEY_LEFTCTRL;
    bk->actions[i].action_keys[1] = KEY_PAGEUP;
    bk->actions[i].action_key_count = 2;
    bk->actions[i].description = strdup("Left gesture: Previous browser tab (Ctrl+PgUp)");
    i++;

    bk->actions[i].direction = DIR_RIGHT;
    bk->actions[i].action_keys[0] = KEY_LEFTCTRL;
    bk->actions[i].action_keys[1] = KEY_PAGEDOWN;
    bk->actions[i].action_key_count = 2;
    bk->actions[i].description = strdup("Right gesture: Next browser tab (Ctrl+PgDown)");
    i++;

    bk->actions[i].direction = DIR_UP;
    bk->actions[i].action_keys[0] = KEY_BRIGHTNESSUP;
    bk->actions[i].action_key_count = 1;
    bk->actions[i].description = strdup("Up gesture: Brightness up");
    i++;

    bk->actions[i].direction = DIR_DOWN;
    bk->actions[i].action_keys[0] = KEY_BRIGHTNESSDOWN;
    bk->actions[i].action_key_count = 1;
    bk->actions[i].description = strdup("Down gesture: Brightness down");
    i++;
    bk->action_count = i;

    /* Default gestures for middle button */
    button_config_t *md = &cfg->buttons[BTN_IDX_MIDDLE];
    i = 0;
    md->actions[i].direction = DIR_NONE;
    md->actions[i].action_keys[0] = KEY_MUTE;
    md->actions[i].action_key_count = 1;
    md->actions[i].description = strdup("Tap: Mute toggle");
    i++;

    md->actions[i].direction = DIR_UP;
    md->actions[i].action_keys[0] = KEY_VOLUMEUP;
    md->actions[i].action_key_count = 1;
    md->actions[i].description = strdup("Up gesture: Volume up");
    i++;

    md->actions[i].direction = DIR_DOWN;
    md->actions[i].action_keys[0] = KEY_VOLUMEDOWN;
    md->actions[i].action_key_count = 1;
    md->actions[i].description = strdup("Down gesture: Volume down");
    i++;
    md->action_count = i;

    /* Default gestures for extra button */
    button_config_t *ex = &cfg->buttons[BTN_IDX_EXTRA];
    i = 0;
    ex->actions[i].direction = DIR_NONE;
    ex->actions[i].action_keys[0] = KEY_PLAYPAUSE;
    ex->actions[i].action_key_count = 1;
    ex->actions[i].description = strdup("Tap: Play/Pause media");
    i++;

    ex->actions[i].direction = DIR_UP;
    ex->actions[i].action_keys[0] = KEY_VOLUMEUP;
    ex->actions[i].action_key_count = 1;
    ex->actions[i].description = strdup("Up gesture: Volume up");
    i++;

    ex->actions[i].direction = DIR_DOWN;
    ex->actions[i].action_keys[0] = KEY_VOLUMEDOWN;
    ex->actions[i].action_key_count = 1;
    ex->actions[i].description = strdup("Down gesture: Volume down");
    i++;

    ex->actions[i].direction = DIR_LEFT;
    ex->actions[i].action_keys[0] = KEY_PREVIOUSSONG;
    ex->actions[i].action_key_count = 1;
    ex->actions[i].description = strdup("Left gesture: Previous track");
    i++;

    ex->actions[i].direction = DIR_RIGHT;
    ex->actions[i].action_keys[0] = KEY_NEXTSONG;
    ex->actions[i].action_key_count = 1;
    ex->actions[i].description = strdup("Right gesture: Next track");
    i++;
    ex->action_count = i;

    /* Default gesture for gesture button */
    button_config_t *gs = &cfg->buttons[BTN_IDX_GESTURE];
    i = 0;
    gs->actions[i].direction = DIR_NONE;
    gs->actions[i].action_keys[0] = KEY_LEFTMETA;
    gs->actions[i].action_key_count = 1;
    gs->actions[i].description = strdup("Tap: App launcher (Super)");
    i++;
    gs->action_count = i;

    /* Default gestures for side button */
    button_config_t *sd = &cfg->buttons[BTN_IDX_SIDE];
    i = 0;
    sd->actions[i].direction = DIR_NONE;
    sd->actions[i].action_keys[0] = KEY_MUTE;
    sd->actions[i].action_key_count = 1;
    sd->actions[i].description = strdup("Tap: Mute/Unmute");
    i++;
    sd->action_count = i;
}

/* --------------------------------------------------------------------------
 * Allocate and init
 * -------------------------------------------------------------------------- */
config_t *config_new(void) {
    config_t *cfg = calloc(1, sizeof(config_t));
    if (!cfg) return NULL;
    config_set_defaults(cfg);
    return cfg;
}

/* --------------------------------------------------------------------------
 * Apply config from a specific file path
 * -------------------------------------------------------------------------- */
int config_load_default(config_t *cfg) {
    char path[1024];
    const char *home = getenv("HOME");

    for (int i = 0; SEARCH_TEMPLATES[i]; i++) {
        if (strstr(SEARCH_TEMPLATES[i], "%s")) {
            if (!home) continue;
            snprintf(path, sizeof(path), SEARCH_TEMPLATES[i], home);
        } else {
            strncpy(path, SEARCH_TEMPLATES[i], sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        }

        if (config_load(cfg, path) == 0) {
            return 0;
        }
    }

    LOG_INFO("No configuration file found, using built-in defaults");
    return 0;
}

/* --------------------------------------------------------------------------
 * Cleanup
 * -------------------------------------------------------------------------- */
void config_free(config_t *cfg) {
    if (!cfg) return;
    for (int i = 0; i < cfg->button_count; i++) {
        free(cfg->buttons[i].name);
        free(cfg->buttons[i].gesture_key);
        for (int j = 0; j < cfg->buttons[i].action_count; j++) {
            free(cfg->buttons[i].actions[j].description);
        }
    }
    free(cfg);
}

/* --------------------------------------------------------------------------
 * Helper: find gesture action for a button+direction
 * -------------------------------------------------------------------------- */
gesture_action_t *config_find_action(config_t *cfg, int button_idx, gesture_dir_t dir) {
    if (button_idx < 0 || button_idx >= cfg->button_count) return NULL;
    button_config_t *b = &cfg->buttons[button_idx];
    for (int i = 0; i < b->action_count; i++) {
        if (b->actions[i].direction == dir) return &b->actions[i];
    }
    return NULL;
}
