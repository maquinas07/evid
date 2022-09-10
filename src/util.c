/**
    util.c - part of evid
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

#include "util.h"
#include "evid.h"

#ifdef HAVE_NOTIFY
#include "libnotify/notify.h"
#include <libnotify/notification.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void verror(const char *errstr, va_list argp) {
#ifdef HAVE_NOTIFY
  char error_notification_summary[20] = {0};
  char error_notification_body[100] = {0};
  snprintf(error_notification_summary, ARR_SIZE(error_notification_summary),
           "%s - error", PROGRAM_NAME);
  vsnprintf(error_notification_body, ARR_SIZE(error_notification_body), errstr,
            argp);
  NotifyNotification *error_notification = notify_notification_new(
      error_notification_summary, error_notification_body, NULL);
  notify_notification_show(error_notification, NULL);
#else
  fprintf(stderr, "%s: ", PROGRAM_NAME);
  vfprintf(stderr, errstr, argp);
#endif
}

void error(const char *errstr, ...) {
  va_list argp;
  va_start(argp, errstr);
  verror(errstr, argp);
  va_end(argp);
}

void die(const char *errstr, ...) {
  va_list argp;
  va_start(argp, errstr);
  verror(errstr, argp);
  va_end(argp);
  raise(SIGINT);
}

void print_version(void) {
  fprintf(stdout, "%s version %s\n", PROGRAM_NAME, PROGRAM_VERSION);
}

void print_usage(void) {
  fprintf(
      stdout,
      "Usage: %s [OPTIONS] \n Available options:\n -i|--info\toutput "
      "runtime info to the console.\n -d|--debug\toutput debug info to the "
      "console.\n -f|--framerate FRAMES\tsets the framerate of the "
      "output video.\n -a|--audio SOURCE\tuses an audio source for the video, "
      "valid options are pulse and alsa with a comma separated input device, "
      "defaults to pulse,default.\n --no-draw-mouse\thides the pointer in the "
      "output video.\n -g|--gif\toutputs the recording to a gif\n "
      "-o|--output\tsaves the recording into this file or directory\n "
      "-v|--version show program version\n"
#ifdef HAVE_ZENITY
      " -z|--use-zenity\tuses a file selection dialog "
      "from zenity instead of a default path to save the output "
      "files\n"
#endif
      ,
      PROGRAM_NAME);
}
