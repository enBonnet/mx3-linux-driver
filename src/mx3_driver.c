/* --------------------------------------------------------------------------
 * mx3_driver.c — Main event loop with CLI argument parsing
 *
 * Usage:
 *   mx3 [options]
 *
 * Options:
 *   -c, --config FILE    Path to configuration file
 *   -d, --device PATH    Force a specific input device (e.g. /dev/input/event5)
 *   -l, --log-level L    debug | info | warn | error (default: info)
 *   --daemon             Fork into background
 *   --pid-file FILE      Write PID to FILE
 *   -v, --version        Print version and exit
 *   -h, --help           Show this help
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>

#include "mx3_common.h"
#include "mx3_device.h"
#include "mx3_uinput.h"
#include "mx3_gesture.h"

static volatile sig_atomic_t g_keep_running = 1;

static void signal_handler(int sig) {
    LOG_INFO("Signal %d received, shutting down...", sig);
    g_keep_running = 0;
}

static void print_usage(const char *prog) {
    printf("MX3 Linux Driver v%s\n\n", MX3_VERSION);
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -c, --config FILE    Path to configuration file\n");
    printf("  -d, --device PATH    Force a specific input device (e.g. /dev/input/event5)\n");
    printf("  -l, --log-level L    debug | info | warn | error (default: info)\n");
    printf("  --daemon             Fork into background\n");
    printf("  --pid-file FILE      Write PID to FILE\n");
    printf("  -v, --version        Print version and exit\n");
    printf("  -h, --help           Show this help\n\n");
    printf("Configuration file search path:\n");
    printf("  1. ./mx3.conf\n");
    printf("  2. $HOME/.config/mx3/config.conf\n");
    printf("  3. $HOME/.config/mx3.conf\n");
    printf("  4. /etc/mx3/config.conf\n\n");
    printf("Default gestures (built-in):\n");
    printf("  Forward button + gesture left:   Super + [   (previous workspace)\n");
    printf("  Forward button + gesture right:  Super + ]   (next workspace)\n");
    printf("  Forward button + gesture up:     Volume Up\n");
    printf("  Forward button + gesture down:   Volume Down\n");
    printf("  Forward button + tap:            Super       (app launcher)\n");
    printf("  Back button + gesture left:      Ctrl+PgUp   (prev tab)\n");
    printf("  Back button + gesture right:     Ctrl+PgDown (next tab)\n");
    printf("  Back button + gesture up:        Brightness Up\n");
    printf("  Back button + gesture down:      Brightness Down\n");
    printf("  Middle button + tap:             Mute toggle\n");
    printf("  Extra button + gesture left:     Previous track\n");
    printf("  Extra button + gesture right:    Next track\n");
}

static int find_button_index(config_t *cfg, int event_code) {
    for (int i = 0; i < cfg->button_count; i++) {
        if (cfg->buttons[i].event_code == event_code) return i;
    }
    return -1;
}

static int open_device_direct(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("Cannot open device %s: %s", path, strerror(errno));
        return -1;
    }
    LOG_INFO("Using forced device: %s (%s)", path, device_name_from_fd(fd));
    return fd;
}

/* --------------------------------------------------------------------------
 * Main — orchestrate everything
 * -------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
    const char *config_path = NULL;
    const char *device_path = NULL;
    log_level_t log_level = LOG_INFO;
    bool daemonize = false;
    const char *pid_file = NULL;

    /* CLI option parsing */
    struct option long_opts[] = {
        {"config",    required_argument, 0, 'c'},
        {"device",    required_argument, 0, 'd'},
        {"log-level", required_argument, 0, 'l'},
        {"daemon",    no_argument,       0,  0 },
        {"pid-file",  required_argument, 0,  0 },
        {"version",   no_argument,       0, 'v'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index;
    while ((opt = getopt_long(argc, argv, "c:d:l:vh", long_opts, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                config_path = optarg;
                break;
            case 'd':
                device_path = optarg;
                break;
            case 'l':
                if (strcmp(optarg, "debug") == 0) log_level = LOG_DEBUG;
                else if (strcmp(optarg, "info") == 0) log_level = LOG_INFO;
                else if (strcmp(optarg, "warn") == 0) log_level = LOG_WARN;
                else if (strcmp(optarg, "error") == 0) log_level = LOG_ERROR;
                else {
                    fprintf(stderr, "Unknown log level: %s\n", optarg);
                    return 1;
                }
                break;
            case 'v':
                printf("MX3 Linux Driver v%s\n", MX3_VERSION);
                return 0;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 0:
                /* long-only options */
                if (strcmp(long_opts[option_index].name, "daemon") == 0) {
                    daemonize = true;
                } else if (strcmp(long_opts[option_index].name, "pid-file") == 0) {
                    pid_file = optarg;
                }
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    mx3_log_set_level(log_level);

    LOG_INFO("MX3 Linux Driver v%s starting", MX3_VERSION);

    /* Allocate and load configuration */
    config_t *cfg = config_new();
    if (!cfg) {
        LOG_ERROR("Failed to allocate configuration");
        return 1;
    }

    if (config_path) {
        if (config_load(cfg, config_path) != 0) {
            LOG_ERROR("Failed to load config file '%s'", config_path);
        }
    } else {
        config_load_default(cfg);
    }

    /* Override log level from config if specified on CLI */
    if (log_level != cfg->log_level) {
        cfg->log_level = log_level;
        mx3_log_set_level(log_level);
    }

    /* Daemonize */
    if (daemonize) {
        LOG_INFO("Daemonizing...");
        pid_t pid = fork();
        if (pid < 0) {
            LOG_ERROR("fork() failed: %s", strerror(errno));
            config_free(cfg);
            return 1;
        }
        if (pid > 0) {
            /* parent — write PID and exit */
            if (pid_file) {
                FILE *pf = fopen(pid_file, "w");
                if (pf) {
                    fprintf(pf, "%d\n", pid);
                    fclose(pf);
                }
            }
            LOG_INFO("Daemon started with PID %d", pid);
            config_free(cfg);
            return 0;
        }
        /* child continues */
        setsid();
        fclose(stdin);
        fclose(stdout);
    } else if (pid_file) {
        FILE *pf = fopen(pid_file, "w");
        if (pf) {
            fprintf(pf, "%d\n", getpid());
            fclose(pf);
        }
    }

    /* Open mouse device */
    int mouse_fd;
    if (device_path) {
        mouse_fd = open_device_direct(device_path);
    } else {
        mouse_fd = device_find_mouse(&cfg->device_match);
    }

    if (mouse_fd < 0) {
        config_free(cfg);
        return 1;
    }

    /* Set non-blocking initially for detection, then switch to blocking */
    int flags = fcntl(mouse_fd, F_GETFL, 0);
    fcntl(mouse_fd, F_SETFL, flags & ~O_NONBLOCK);

    /* Create uinput device */
    int uinput_fd = uinput_create();
    if (uinput_fd < 0) {
        LOG_ERROR("Failed to create virtual keyboard device. Check permissions on /dev/uinput.");
        close(mouse_fd);
        config_free(cfg);
        return 1;
    }

    /* Set up signal handlers */
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP,  signal_handler);

    /* Gesture tracking state */
    gesture_tracker_t tracker;
    gesture_tracker_init(&tracker, cfg->motion_threshold);

    LOG_INFO("Monitoring mouse events. Press Ctrl+C to stop.");

    /* ----------------------------------------------------------------------
     * Main event loop
     * ---------------------------------------------------------------------- */
    struct input_event ev;
    while (g_keep_running) {
        ssize_t n = read(mouse_fd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("Error reading from mouse device: %s", strerror(errno));
            break;
        }
        if (n != sizeof(ev)) continue;

        if (ev.type == EV_KEY) {
            int btn_idx = find_button_index(cfg, ev.code);

            if (btn_idx >= 0) {
                button_config_t *btn = &cfg->buttons[btn_idx];

                if (ev.value == 1) {
                    /* button pressed — start gesture tracking */
                    gesture_tracker_init(&tracker, btn->motion_threshold);
                    gesture_tracker_start(&tracker, ev.code);
                    LOG_DEBUG("Button '%s' (0x%x) pressed", btn->name, ev.code);
                } else if (ev.value == 0) {
                    /* button released — resolve gesture */
                    int btn_code = gesture_tracker_button(&tracker);

                    if (btn_code == (int)ev.code) {
                        double duration = 0;
                        gesture_dir_t dir = gesture_tracker_resolve(&tracker, &duration);

                        LOG_DEBUG("Button '%s' released: gesture=%s, duration=%.2fs, dx=%d, dy=%d",
                                  btn->name, gesture_dir_name(dir), duration,
                                  tracker.accum_x, tracker.accum_y);

                        gesture_action_t *action = NULL;
                        for (int i = 0; i < btn->action_count; i++) {
                            if (btn->actions[i].direction == dir) {
                                action = &btn->actions[i];
                                break;
                            }
                        }

                        if (action && action->action_key_count > 0) {
                            if (action->description) {
                                LOG_INFO("Executing: %s", action->description);
                            }
                            uinput_send_keys(uinput_fd, action->action_keys, action->action_key_count);
                        } else {
                            LOG_DEBUG("No action defined for gesture '%s' on button '%s'",
                                      gesture_dir_name(dir), btn->name);
                        }
                    }

                    gesture_tracker_stop(&tracker);
                }
            }
        } else if (ev.type == EV_REL && gesture_tracker_is_active(&tracker)) {
            if (ev.code == REL_X) {
                gesture_tracker_add_motion(&tracker, ev.value, 0);
            } else if (ev.code == REL_Y) {
                gesture_tracker_add_motion(&tracker, 0, ev.value);
            }
        }
    }

    /* Cleanup */
    uinput_destroy(uinput_fd);
    close(mouse_fd);

    if (pid_file) unlink(pid_file);

    config_free(cfg);
    LOG_INFO("MX3 Linux Driver terminated.");

    return 0;
}
