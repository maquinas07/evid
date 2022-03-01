/* xrectsel.c -- print the geometry of a rectangular screen region.
   Slightly modified version, copyright and license notices are unchanged.

   Copyright (C) 2011-2014  lolilolicon <lolilolicon@gmail.com>
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "xrectsel.h"
#include "actions.h"
#include "util.h"
#include "x11_grab.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#ifdef HAVE_XEXTENSIONS
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#endif

int select_region(Display *dpy, Window root, _Region *region) {
  _Region rr; /* root region */
  _Region sr; /* selected region */

  {
    Window wroot;
    if (False == XGetGeometry(dpy, root, &wroot, &rr.x, &rr.y, &rr.w, &rr.h,
                              &rr.b, &rr.d)) {
      error("failed to get root window geometry\n");
      return -1;
    }
  }

  XVisualInfo vinfo;
  XMatchVisualInfo(dpy, DefaultScreen(dpy), 32, TrueColor, &vinfo);

  XSetWindowAttributes wa;
  wa.colormap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
  wa.override_redirect = True;
  wa.border_pixel = 0;
  wa.background_pixel = 0xB0000000;

  Window w = XCreateWindow(
      dpy, root, rr.x, rr.y, rr.w, rr.h, rr.b, vinfo.depth, InputOutput,
      vinfo.visual,
      CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect, &wa);
  Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", True);
  Atom wm_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN, ", True);
  Atom wm_above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", True);
  Atom wm_stays_on_top = XInternAtom(dpy, "_NET_WM_STATE_STAYS_ON_TOP", True);
  XChangeProperty(dpy, w, wm_state, XA_ATOM, 32, PropModeReplace,
                  (const unsigned char *)&wm_fullscreen, 1);
  XChangeProperty(dpy, w, wm_state, XA_ATOM, 32, PropModeAppend,
                  (const unsigned char *)&wm_above, 1);
  XChangeProperty(dpy, w, wm_state, XA_ATOM, 32, PropModeAppend,
                  (const unsigned char *)&wm_stays_on_top, 1);

#ifdef HAVE_XEXTENSIONS
  XRectangle rect;
  XserverRegion reg = XFixesCreateRegion(dpy, &rect, 1);
  XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, reg);
  XFixesDestroyRegion(dpy, reg);
#endif

  XSizeHints *xh = XAllocSizeHints();
  xh->width = rr.w;
  xh->height = rr.h;
  xh->win_gravity = StaticGravity;
  XSetWMNormalHints(dpy, w, xh);

  XMapWindow(dpy, w);

  GC sel_gc;
  XGCValues sel_gv;

  int status, done = 0, btn_pressed = 0;
  int x = 0, y = 0;
  unsigned int width = 0, height = 0;
  int start_x = 0, start_y = 0;

  Cursor cursor;
  cursor = XCreateFontCursor(dpy, XC_tcross);

  /* Grab pointer for these events */
  status = XGrabPointer(
      dpy, root, True, PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
      GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);

  if (status != GrabSuccess) {
    error("failed to grab pointer\n");
    return -1;
  }

  sel_gv.subwindow_mode = IncludeInferiors;
  sel_gv.line_width = 2;
  sel_gc = XCreateGC(dpy, w, GCSubwindowMode | GCLineWidth, &sel_gv);

  grab_keys(dpy, &root, 0);

#ifdef HAVE_XEXTENSIONS
  Window clicked_window = 0;
#endif
  XEvent event;
  for (;;) {
    XNextEvent(dpy, &event);
    switch (event.type) {
    case KeyPress:
    case KeyRelease: {
      if (event.xkey.keycode == ABORT_KEYCODE(dpy) &&
          (event.xkey.state &
           (ShiftMask | ControlMask | Mod1Mask | Mod4Mask)) == ABORT_MODFIELD) {
        ungrab_keys(dpy, &root, 0);
        return -2;
      }
    }
    case ButtonPress: {
      btn_pressed = 1;
      x = start_x = event.xbutton.x_root;
      y = start_y = event.xbutton.y_root;
      width = height = 0;
      break;
    }
    case MotionNotify: {
      /* Draw only if button is pressed */
      if (btn_pressed) {
        XClearWindow(dpy, w);

        x = event.xbutton.x_root;
        y = event.xbutton.y_root;

        if (x > start_x) {
          width = x - start_x;
          x = start_x;
        } else {
          width = start_x - x;
        }
        if (y > start_y) {
          height = y - start_y;
          y = start_y;
        } else {
          height = start_y - y;
        }

        /* Draw Rectangle */
        XFillRectangle(dpy, w, sel_gc, x, y, width, height);
        XFlush(dpy);
      }
      break;
    }
    case ButtonRelease: {
#ifdef HAVE_XEXTENSIONS
      clicked_window = event.xbutton.subwindow;
#endif
      done = 1;
      break;
    }
    default:
      break;
    }
    if (done) {
      break;
    }
  }

  ungrab_keys(dpy, &root, 0);
  XUngrabPointer(dpy, CurrentTime);
  XFreeCursor(dpy, cursor);
  XFreeGC(dpy, sel_gc);
  XFree(xh);
  XSync(dpy, 1);

  XDestroyWindow(dpy, w);
  XFlush(dpy);

  if (!width && !height && clicked_window) {
    XWindowAttributes attrs;
    XGetWindowAttributes(dpy, clicked_window, &attrs);
    Window parent = 0;
    Window *children = 0;
    unsigned int nchildren;
    XQueryTree(dpy, clicked_window, &root, &parent, &children, &nchildren);
    if (children) {
      XFree(children);
    }
    if (parent == attrs.root) {
      x = attrs.x;
      y = attrs.y;
    } else {
      Window stub;
      XTranslateCoordinates(dpy, clicked_window, attrs.root, 0, 0, &x, &y,
                            &stub);
    }
    sr.x = x;
    sr.y = y;
    sr.w = attrs.width;
    sr.h = attrs.height;
  } else {
    sr.x = x;
    sr.y = y;
    sr.w = width;
    sr.h = height;
  }

  /* calculate right and bottom offset */
  sr.X = rr.w - sr.x - sr.w;
  sr.Y = rr.h - sr.y - sr.h;
  /* those doesn't really make sense but should be set */
  sr.b = rr.b;
  sr.d = rr.d;
  *region = sr;

  return 0;
}
