#include "util.h"
#include <libnotify/notification.h>

#ifdef HAVE_NOTIFY
#include "libnotify/notify.h"
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void error(const char *errstr, ...) {
  va_list ap;
  fprintf(stderr, "%s: ", PROGRAM_NAME);
  va_start(ap, errstr);
  vfprintf(stderr, errstr, ap);

#ifdef HAVE_NOTIFY
  char error_notification_summary[50];
  snprintf(error_notification_summary, ARR_SIZE(error_notification_summary),
           "%s: %s", PROGRAM_NAME, errstr);
  NotifyNotification *error_notification =
      notify_notification_new(error_notification_summary, NULL, NULL);
  notify_notification_show(error_notification, NULL);
#endif

  va_end(ap);
}

void die(const char *errstr, ...) {
  va_list args;

  va_start(args, errstr);
  error(errstr, args);
  va_end(args);

  exit(1);
}

void print_usage() {
  fprintf(
      stdout,
      "Usage: %s [OPTIONS] \n Available options:\n -i|--info\toutput "
      "runtime info to the console.\n -d|--debug\toutput debug info to the "
      "console.\n -f|--framerate FRAMES\tsets the framerate of the "
      "output video.\n -a|--audio SOURCE\tuses an audio source for the video, "
      "valid options are pulse and alsa, defaults to alsa.\n "
      "--no-draw-mouse\thides the pointer in the output video.\n "
      "-g|--gif\toutputs the recording to a gif\n -o|--output\tsaves the "
      "recording into this file or directory"
#ifdef HAVE_ZENITY
      " -z|--use-zenity\tuses a file selection dialog "
      "from zenity instead of a default path to save the output "
      "files\n"
#endif
      ,
      PROGRAM_NAME);
}
