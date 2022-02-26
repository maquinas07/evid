#ifndef ACTIONS_EVID_H
#define ACTIONS_EVID_H

#include "X11/keysym.h"

#define ABORT 1 << 7
#define SAVE 1 << 1
#define COPY 1 << 0

#define ABORT_KEYSYM XK_Escape
#define SAVE_KEYSYM XK_s
#define COPY_KEYSYM XK_c

#define ABORT_MODFIELD 0
#define SAVE_MODFIELD ControlMask
#define COPY_MODFIELD ControlMask

#define ABORT_KEYCODE(dpy) XKeysymToKeycode(dpy, ABORT_KEYSYM)
#define SAVE_KEYCODE(dpy) XKeysymToKeycode(dpy, SAVE_KEYSYM)
#define COPY_KEYCODE(dpy) XKeysymToKeycode(dpy, COPY_KEYSYM)

#endif
