#ifndef EVID_X11GRAB_H
#define EVID_X11GRAB_H
#include <X11/Xlib.h>

void update_active_window(Window *active_window, Display *dpy, Window *root,
                          Atom *net_active_window);
void grab_keys(Display *dpy, Window *window, unsigned char actions);
void ungrab_keys(Display *dpy, Window *window, unsigned char actions);
int handle_x_error(Display *dpy, XErrorEvent *error);

#endif
