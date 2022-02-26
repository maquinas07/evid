#ifndef XRECTSEL_H
#define XRECTSEL_H

#include "types.h"
#include <X11/Xlib.h>

int select_region(Display *dpy, Window root, _Region *region);

#endif
