#include "x11_grab.h"
#include "actions.h"
#include "util.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>

unsigned int special_modifiers[] = {0, Mod2Mask, LockMask,
                                    (Mod2Mask | LockMask)};

void update_active_window(Window *active_window, Display *dpy, Window *root,
                          Atom *net_active_window) {
  Atom actual_type;
  int actual_format;
  unsigned long nItems;
  unsigned long bytesAfter;
  unsigned char *prop;
  if (XGetWindowProperty(dpy, *root, *net_active_window, 0, 1024, False,
                         AnyPropertyType, &actual_type, &actual_format, &nItems,
                         &bytesAfter, &prop) == Success) {
    *active_window = *(unsigned long *)prop;
    XFree(prop);
  }
}

void grab_keys(Display *dpy, Window *window, unsigned char actions) {
  XSetErrorHandler(&handle_x_error);
  for (unsigned int mod = 0; mod < ARR_SIZE(special_modifiers); ++mod) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, ABORT_KEYSYM),
             ABORT_MODFIELD | special_modifiers[mod], *window, False,
             GrabModeSync, GrabModeSync);
    if ((SAVE & actions) == SAVE) {
      XGrabKey(dpy, XKeysymToKeycode(dpy, SAVE_KEYSYM),
               SAVE_MODFIELD | special_modifiers[mod], *window, True,
               GrabModeSync, GrabModeSync);
    }
    if ((COPY & actions) == COPY) {
      XGrabKey(dpy, XKeysymToKeycode(dpy, COPY_KEYSYM),
               COPY_MODFIELD | special_modifiers[mod], *window, True,
               GrabModeSync, GrabModeSync);
    }
  }
  XSetErrorHandler(NULL);
}

void ungrab_keys(Display *dpy, Window *active_window, unsigned char actions) {
  XSetErrorHandler(&handle_x_error);
  for (unsigned int mod = 0; mod < ARR_SIZE(special_modifiers); mod++) {
    XUngrabKey(dpy, XKeysymToKeycode(dpy, ABORT_KEYSYM),
               ABORT_MODFIELD | special_modifiers[mod], *active_window);
    XUngrabKey(dpy, XKeysymToKeycode(dpy, SAVE_KEYSYM),
               SAVE_MODFIELD | special_modifiers[mod], *active_window);
    XUngrabKey(dpy, XKeysymToKeycode(dpy, COPY_KEYSYM),
               COPY_MODFIELD | special_modifiers[mod], *active_window);
  }
  XSetErrorHandler(NULL);
}

int handle_x_error(Display *dpy, XErrorEvent *error) {
  // Ignore BadAccess error on grabs
  switch (error->error_code) {
  case BadAccess: {
    if (error->request_code == 33 /* grab key */
        || error->request_code == 34 /* ungrab_key */) {
      die("failed to grab needed keys\n");
      return 0;
    }
    break;
  }
  default: {
    int error_size = 50;
    char error_msg[error_size];
    XGetErrorText(dpy, error->error_code, error_msg, error_size);
    die(error_msg);
  }
  }
  return 1;
}
