/**
    actions.h - part of evid
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
