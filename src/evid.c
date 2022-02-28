#include "actions.h"
#include "file.h"
#include "types.h"
#include "util.h"
#include "x11_grab.h"
#include "xrectsel.h"

#ifdef HAVE_NOTIFY
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

#define PROGRAM_NAME "evid"

#define GRAB(dpy, window) grab_keys(dpy, window, SAVE)
#define UNGRAB(dpy, window) ungrab_keys(dpy, window, SAVE)

#define QUIET 0
#define INFO 1
#define DEBUG 2

#define LQGIF 1
#define HQGIF 2

static void process_args(Args *args, int argc, char **argv) {
#ifdef HAVE_ZENITY
  args->use_zenity = 0;
#endif
  args->framerate = "10";
  args->audio = NULL;
  args->verbosity = QUIET;
  args->draw_mouse = 1;
  args->show_region = 0;
  args->gif = 0;
  args->output = NULL;

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
      {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "idzgo:f:a::h", long_opts, NULL)) !=
         -1) {
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
      char *default_audio[] = {"alsa", "-ac", "2", "-i", "default", NULL};
      if (optarg) {
        if (!strcmp(optarg, "pulse")) {
          default_audio[0] = "pulse";
        } else if (!strcmp(optarg, "alsa")) {
          print_usage();
          exit(EXIT_FAILURE);
        }
      }
      args->audio = default_audio;
      break;
    }
    case ('h'): {
      print_usage();
      exit(EXIT_SUCCESS);
    }
    default: {
      print_usage();
      exit(EXIT_FAILURE);
    }
    }
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
  if (pid < 0) {
    die("error forking the current process\n");
  } else if (pid == 0) {
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
  int status = 0;
  if (wait(&status) == -1 || status) {
    remove_file(source);
    die("failed to generate gif from %s\n", source);
  }
}

static int exec_ffmpeg(Args *args, _Region selected_region, char *tmp_file) {
  int fargsc = 0;
  char *fargs[50];
  fargs[fargsc++] = "ffmpeg";
  fargs[fargsc++] = "-y";
  fargs[fargsc++] = "-f";
  fargs[fargsc++] = "x11grab";
  if (args->audio && !args->gif) {
    fargs[fargsc++] = "-f";
    for (unsigned int i = 0; args->audio[i]; i++) {
      fargs[fargsc++] = args->audio[i];
    }
  }
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

static unsigned char run_supervise_loop(pid_t sub_process_pid, Display *dpy,
                                        Window *root, int *status) {
  unsigned char action = 0;

  XSetWindowAttributes wa;
  wa.event_mask = PropertyChangeMask;
  XChangeWindowAttributes(dpy, *root, CWEventMask, &wa);

  Atom net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  Window active_window = 0;

  update_active_window(&active_window, dpy, root, &net_active_window);
  GRAB(dpy, &active_window);

  while (!waitpid(sub_process_pid, status, WNOHANG)) {
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
        if (!action) {
          XAllowEvents(dpy, ReplayKeyboard, event.xkey.time);
          XFlush(dpy);
        } else {
          kill(sub_process_pid, SIGTERM);
        }
      }
      }
    }
  }

  UNGRAB(dpy, &active_window);
  return action;
}

int main(int argc, char *argv[]) {
#ifdef HAVE_NOTIFY
  notify_init(PROGRAM_NAME);
#endif

  // Force to use x11
  setenv("GDK_BACKEND", "x11", 1);

  Args args = {0};
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

  char tmp_file[FILENAME_MAX];
  if (get_tmp_file(tmp_file, sizeof(tmp_file), &args) <= 0) {
    die("failed to get a temporary file\n");
  }

  pid_t pid = fork();
  if (pid < 0) {
    die("error forking the current process\n");
  } else if (pid == 0) {
    int ffmpeg_err = exec_ffmpeg(&args, selected_region, tmp_file);
    if (ffmpeg_err) {
      die("failed to launch ffmpeg, error: %s\n", strerror(errno));
    }
  } else {
    int status = 0;
    unsigned char action = run_supervise_loop(pid, dpy, &root, &status);
    XCloseDisplay(dpy);

    if (WEXITSTATUS(status) == EXIT_FAILURE) {
      return EXIT_FAILURE;
    }

    if (WEXITSTATUS(status) == EXIT_SUCCESS ||
        (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM) ||
        WEXITSTATUS(status) == 0xFF) {
      switch (action) {
      case SAVE: {
        char new_file[PATH_MAX];
        if (get_output_file(new_file, sizeof(new_file), &args) < 0) {
          remove_file(tmp_file);
#ifdef HAVE_ZENITY
          die("couldn't save the file in the default location, install "
              "zenity or "
              "define either the HOME or XDG_VIDEOS_DIR environment "
              "variables\n");
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

#ifdef HAVE_NOTIFY
        char success_notification_summary[50];
        snprintf(success_notification_summary,
                 ARR_SIZE(success_notification_summary), "%s: %s", PROGRAM_NAME,
                 "recording saved successfuly");
        NotifyNotification *success_notification = notify_notification_new(
            success_notification_summary, new_file, NULL);
        notify_notification_show(success_notification, NULL);
#endif
      }
      case COPY:
      // Seems unnecesary for now since no application I know of handles mp4
      // or gif mime-types
      default:
        break;
      }
    }
  }

  remove_file(tmp_file);

#ifdef HAVE_NOTIFY
  notify_uninit();
#endif
  return EXIT_SUCCESS;
}
