/**
    evid.c - screen recorder for X with a minimal interface
    Copyright (C) 2022  Elias Menon

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#include "evid.h"
#include "actions.h"
#include "clipboard.h"
#include "file.h"
#include "types.h"
#include "util.h"
#include "x11_grab.h"
#include "xrectsel.h"

#ifdef HAVE_NOTIFY
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bits/getopt_core.h>
#include <linux/limits.h>
#include <sys/wait.h>

#define GRAB(dpy, window) grab_keys(dpy, window, SAVE | COPY)
#define UNGRAB(dpy, window) ungrab_keys(dpy, window, SAVE | COPY)

#define QUIET 0
#define INFO 1
#define DEBUG 2

#define LQGIF 1
#define HQGIF 2

static pid_t subp = 0;
static char tmp_file[FILENAME_MAX] = {0};

static void shutdown(int signo) {
  if (subp) {
    kill(subp, signo);
    sleep(1);
    if (!waitpid(subp, NULL, WNOHANG)) {
      kill(SIGKILL, subp);
      waitpid(subp, NULL, 0);
    }
  }
  if (ARR_SIZE(tmp_file) > 0) {
    remove_file(tmp_file);
  }
  exit(EXIT_FAILURE);
}

static void process_args(Args *args, int argc, char **argv) {
  int option_index = 0;
  struct option long_opts[] = {
      {"info", no_argument, &args->verbosity, INFO},
      {"debug", no_argument, &args->verbosity, DEBUG},
#ifdef HAVE_ZENITY
      {"use-zenity", no_argument, &args->use_zenity, 1},
#endif
      {"gif", no_argument, &args->gif, LQGIF},
      {"show-region", no_argument, &args->show_region, 1},
      {"no-draw-mouse", no_argument, &args->draw_mouse, 0},
      {"framerate", required_argument, NULL, 'f'},
      {"audio", optional_argument, NULL, 'a'},
      {"output", required_argument, NULL, 'o'},
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "idzgo:f:a::hv", long_opts,
                            &option_index)) != -1) {
    switch (opt) {
    case ('i'): {
      args->verbosity = INFO;
      break;
    }
    case ('d'): {
      args->verbosity = DEBUG;
      break;
    }
#ifdef HAVE_ZENITY
    case ('z'): {
      args->use_zenity = 1;
      break;
    }
#endif
    case ('g'): {
      if (args->gif) {
        args->gif++;
      } else {
        args->gif = LQGIF;
      }
      break;
    }
    case ('o'): {
      args->output = optarg;
      break;
    }
    case ('f'): {
      args->framerate = optarg;
      break;
    }
    case ('a'): {
      args->audio->subsystem = "pulse";
      args->audio->input = "default";
      if (optarg) {
        char *subsystem = strtok(optarg, ",");
        char *input = strtok(NULL, ",");

        if (!strcmp(subsystem, "alsa")) {
          args->audio->subsystem = subsystem;
        } else if (strcmp(optarg, "pulse")) {
          print_usage();
          exit(EXIT_FAILURE);
        }
        if (input) {
          args->audio->input = input;
        }
      }
      break;
    }
    case ('v'): {
      print_version();
      exit(EXIT_SUCCESS);
    }
    case ('h'): {
      print_usage();
      exit(EXIT_SUCCESS);
    }
    default: {
      if (long_opts[option_index].flag) {
        break;
      }
      print_usage();
      exit(EXIT_FAILURE);
    }
    }
  }

  if (args->verbosity == DEBUG) {
    printf("Parsed arguments: \n\tGif: %d\n\tFramerate: "
           "%s\n\tShow region: %d\n\tAudio subsystem: %s\n\tAudio input "
           "device: %s\n\tOutput: %s\n"
#ifdef HAVE_ZENITY
           "\tUse zenity: %d"
#endif
           ,
           args->gif, args->framerate, args->show_region,
           args->audio->subsystem, args->audio->input, args->output
#ifdef HAVE_ZENITY
           ,
           args->use_zenity
#endif
    );
  }
}

static void set_verbose(char **fargs, int verbose) {
  if (verbose == DEBUG) {
    fprintf(stdout, "Executing ffmpeg with command: ");
    char **i = fargs;
    for (; *i; ++i) {
      fprintf(stdout, "%s ", *i);
    }
    fprintf(stdout, "\n");
    *i++ = "-loglevel";
    *i++ = "debug";
    *i = NULL;
  } else if (verbose == QUIET) {
    close(2);
    open("/dev/null", O_RDWR);
  }
}

static void create_gif(Args *args, char *source, char *dest) {
  pid_t pid = fork();
  switch (pid) {
  case -1: {
    die("error forking the current process\n");
  }
  case 0: {
    int fargsc = 0;
    char *fargs[50];
    fargs[fargsc++] = "ffmpeg";
    fargs[fargsc++] = "-y";
    fargs[fargsc++] = "-i";
    fargs[fargsc++] = source;
    fargs[fargsc++] = "-vf";
    fargs[fargsc++] = "split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse";
    fargs[fargsc++] = dest;
    fargs[fargsc] = NULL;
    set_verbose(fargs, args->verbosity);
    execvp(fargs[0], fargs);
  }
  default: {
    int status = 0;
    if (wait(&status) == -1 || status) {
      remove_file(source);
      die("failed to generate gif from %s\n", source);
    }
  }
  }
}

static int exec_ffmpeg(Args *args, _Region selected_region, char *tmp_file) {
  int fargsc = 0;
  char *fargs[50];
  fargs[fargsc++] = "ffmpeg";
  fargs[fargsc++] = "-y";
  if (args->audio->subsystem && args->audio->input && !args->gif) {
    fargs[fargsc++] = "-f";
    fargs[fargsc++] = args->audio->subsystem;
    fargs[fargsc++] = "-ac";
    fargs[fargsc++] = "2";
    fargs[fargsc++] = "-i";
    fargs[fargsc++] = args->audio->input;
    fargs[fargsc++] = "-acodec";
    fargs[fargsc++] = "aac";
  }
  fargs[fargsc++] = "-f";
  fargs[fargsc++] = "x11grab";
  fargs[fargsc++] = "-video_size";
  char video_size[10];
  fargs[fargsc] = video_size;
  snprintf(fargs[fargsc++], sizeof(video_size), "%dx%d", selected_region.w,
           selected_region.h);
  if (args->show_region) {
    fargs[fargsc++] = "-show_region";
    fargs[fargsc++] = "1";
  }
  if (args->framerate) {
    fargs[fargsc++] = "-framerate";
    fargs[fargsc++] = args->framerate;
  }
  if (!args->draw_mouse) {
    fargs[fargsc++] = "-draw_mouse";
    fargs[fargsc++] = "0";
  }
  fargs[fargsc++] = "-i";
  char *display = getenv("DISPLAY");
  if (!display) {
    display = ":0";
  }
  char input_display[15];
  fargs[fargsc] = input_display;
  snprintf(fargs[fargsc++], sizeof(input_display), "%s+%d,%d", display,
           selected_region.x, selected_region.y);
  if (args->gif == LQGIF) {
    fargs[fargsc++] = "-vf";
    fargs[fargsc++] = "scale=-2:-2:flags=lanczos";
  } else {
    fargs[fargsc++] = "-c:v";
    fargs[fargsc++] = "libx264";
    fargs[fargsc++] = "-vf";
    fargs[fargsc++] = "crop=trunc(iw/2)*2:trunc(ih/2)*2";
    fargs[fargsc++] = "-preset";
    fargs[fargsc++] = "superfast";
    fargs[fargsc++] = "-crf";
    fargs[fargsc++] = "18";
    fargs[fargsc++] = "-pix_fmt";
    fargs[fargsc++] = "yuv420p";
    if (args->gif == HQGIF) {
      fargs[fargsc++] = "-f";
      fargs[fargsc++] = "h264";
    }
  }
  fargs[fargsc++] = tmp_file;
  fargs[fargsc] = NULL;
  set_verbose(fargs, args->verbosity);
  return execvp(fargs[0], fargs);
}

static unsigned char get_matching_action(Display *dpy, XKeyEvent event) {
  unsigned int modfield =
      event.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);
  if (event.keycode == ABORT_KEYCODE(dpy) && modfield == ABORT_MODFIELD) {
    return ABORT;
  }
  if (event.keycode == SAVE_KEYCODE(dpy) && modfield == SAVE_MODFIELD) {
    return SAVE;
  }
  if (event.keycode == COPY_KEYCODE(dpy) && modfield == COPY_MODFIELD) {
    return COPY;
  }
  return 0;
}

static unsigned char run_supervise_loop(pid_t process_pid, Display *dpy,
                                        Window *root, int *status) {
  unsigned char action = 0;

  XSetWindowAttributes wa = {0};
  wa.event_mask = PropertyChangeMask;
  XChangeWindowAttributes(dpy, *root, CWEventMask, &wa);

  Atom net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  Window active_window = 0;

  update_active_window(&active_window, dpy, root, &net_active_window);
  GRAB(dpy, &active_window);

  while (!waitpid(process_pid, status, WNOHANG)) {
    while (XPending(dpy)) {
      XEvent event;
      XNextEvent(dpy, &event);
      switch (event.type) {
      case PropertyNotify: {
        if (event.xproperty.atom == net_active_window) {
          UNGRAB(dpy, &active_window);
          update_active_window(&active_window, dpy, root, &net_active_window);
          GRAB(dpy, &active_window);
        }
        break;
      }
      case KeyPress:
      case KeyRelease: {
        action = get_matching_action(dpy, event.xkey);
        if (action) {
          kill(process_pid, SIGTERM);
        } else {
          XAllowEvents(dpy, ReplayKeyboard, event.xkey.time);
          XFlush(dpy);
        }
        break;
      }
      }
    }
  }

  UNGRAB(dpy, &active_window);
  XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
  XSync(dpy, True);

  return action;
}

static int notify_cancel() {
#ifdef HAVE_NOTIFY
  char success_notification_summary[50];
  snprintf(success_notification_summary, ARR_SIZE(success_notification_summary),
           "%s: %s", PROGRAM_NAME, "recording was cancelled");
  NotifyNotification *success_notification =
      notify_notification_new(success_notification_summary, NULL, NULL);
  int result = notify_notification_show(success_notification, NULL);
  g_object_unref(success_notification);
  return result;
#endif
  return 0;
}

#ifdef HAVE_NOTIFY
static void
show_in_file_manager_callback(NotifyNotification *notification,
                                     char *action, gpointer loop) {
  if (action != NULL) {
    show_file_in_default_file_manager((const char *)action);
  }
  g_main_loop_quit(*((GMainLoop **)loop));
}

static int on_notification_closed(NotifyNotification *notification,
                                  gpointer loop) {
  g_main_loop_quit(*((GMainLoop **)loop));
  return 0;
}

static int on_notification_timeout(gpointer loop) {
  g_main_loop_quit(*((GMainLoop **)loop));
  return 0;
}
#endif

int main(int argc, char *argv[]) {
#ifdef HAVE_NOTIFY
  notify_init(PROGRAM_NAME);
#endif

  // Force to use x11
  setenv("GDK_BACKEND", "x11", 1);

  Args args = {0};
  Audio audio = {0};

#ifdef HAVE_ZENITY
  args.use_zenity = 0;
#endif
  args.framerate = "10";
  args.audio = &audio;
  args.verbosity = QUIET;
  args.draw_mouse = 1;
  args.show_region = 0;
  args.gif = 0;
  args.output = NULL;

  process_args(&args, argc, argv);

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    die("failed to open display %s\n", getenv("DISPLAY"));
  }

  Window root = DefaultRootWindow(dpy);
  _Region selected_region;

  {
    int r = select_region(dpy, root, &selected_region);
    if (r) {
      XCloseDisplay(dpy);
      if (r != -2) {
        die("failed to select a region\n");
      }
      return EXIT_SUCCESS;
    }
  }

  if (selected_region.w == 0 || selected_region.h == 0) {
    XCloseDisplay(dpy);
    die("selected region is empty\n");
  }

  if (get_tmp_file(tmp_file, sizeof(tmp_file), &args) <= 0) {
    die("failed to get a temporary file\n");
  }
  subp = fork();
  switch (subp) {
  case -1: {
    die("error forking the current process\n");
  }
  case 0: {
    exec_ffmpeg(&args, selected_region, tmp_file);
    die("failed to launch ffmpeg, error: %s\n", strerror(errno));
  }
  default: {
    signal(SIGTERM, &shutdown);
    signal(SIGINT, &shutdown);

    int status = 0;
    unsigned char action = run_supervise_loop(subp, dpy, &root, &status);

    if (WEXITSTATUS(status) == EXIT_FAILURE) {
      return EXIT_FAILURE;
    }

    if (WEXITSTATUS(status) == EXIT_SUCCESS ||
        (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM) ||
        WEXITSTATUS(status) == 0xFF) {
      switch (action) {
      case SAVE: {
        char new_file[PATH_MAX];
        int res = get_output_file(new_file, sizeof(new_file), &args);
        if (res < 0) {
          remove_file(tmp_file);
#ifdef HAVE_ZENITY
          if (res == -2) {
            notify_cancel();
            break;
          } else {
            die("couldn't save the file in the default location, install "
                "zenity or "
                "define either the HOME or XDG_VIDEOS_DIR environment "
                "variables\n");
          }
#else
          die("couldn't save the file in the default location"
              "define either the HOME or XDG_VIDEOS_DIR environment "
              "variables or check its permissions\n");
#endif
        }
        if (args.gif == HQGIF) {
          create_gif(&args, tmp_file, new_file);
        } else {
          move_file(tmp_file, new_file);
        }

        remove_file(tmp_file);
#ifdef HAVE_NOTIFY
        GMainLoop *ml = g_main_loop_new(0, 1);
        char success_notification_summary[50];
        snprintf(success_notification_summary,
                 ARR_SIZE(success_notification_summary), "%s: %s", PROGRAM_NAME,
                 "recording saved successfuly");
        NotifyNotification *success_notification = notify_notification_new(
            success_notification_summary, new_file, NULL);
        notify_notification_add_action(
            success_notification, new_file, "success",
            show_in_file_manager_callback, &ml, NULL);
        notify_notification_show(success_notification, NULL);
        int timeout = 5000;
        notify_notification_set_timeout(success_notification, timeout);
        g_signal_connect(success_notification, "closed",
                         G_CALLBACK(on_notification_closed), &ml);
        g_timeout_add(timeout, on_notification_timeout, &ml);
        g_main_loop_run(ml);
        g_main_loop_unref(ml);
        g_object_unref(success_notification);
#endif
        break;
      }
      case COPY: {
        if (!copy_file(tmp_file)) {
#ifdef HAVE_NOTIFY
          char success_notification_summary[50];
          snprintf(success_notification_summary,
                   ARR_SIZE(success_notification_summary), "%s: %s",
                   PROGRAM_NAME, "recording saved to clipboard");
          NotifyNotification *success_notification =
              notify_notification_new(success_notification_summary, NULL, NULL);
          notify_notification_show(success_notification, NULL);
          g_object_unref(success_notification);
#endif
          break;
        }
      }
      default: {
        notify_cancel();
        break;
      }
      }
    }
  }
  }
#ifdef HAVE_NOTIFY
  notify_uninit();
#endif
  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
