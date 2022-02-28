/**
    x11_grab.h - part of evid
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

#ifndef EVID_X11GRAB_H
#define EVID_X11GRAB_H
#include <X11/Xlib.h>

void update_active_window(Window *active_window, Display *dpy, Window *root,
                          Atom *net_active_window);
void grab_keys(Display *dpy, Window *window, unsigned char actions);
void ungrab_keys(Display *dpy, Window *window, unsigned char actions);
int handle_x_error(Display *dpy, XErrorEvent *error);

#endif
