#include "clipboard.h"
#include "file.h"
#include "util.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { XA_TARGETS, XA_URI_LIST, XA_GNOME_COPY, XA_LAST };

static void deny_selection_request(Display *dpy, XSelectionRequestEvent *sev) {
  XSelectionEvent ssev;

  ssev.type = SelectionNotify;
  ssev.requestor = sev->requestor;
  ssev.selection = sev->selection;
  ssev.target = sev->target;
  ssev.property = None;
  ssev.time = sev->time;

  XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

static void send_targets_list(Display *dpy, XSelectionRequestEvent *sev,
                              Atom targets, Atom *atoms, int natoms) {

  XSelectionEvent ssev;

  XChangeProperty(dpy, sev->requestor, sev->property, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)atoms, natoms);

  ssev.type = SelectionNotify;
  ssev.requestor = sev->requestor;
  ssev.selection = sev->selection;
  ssev.target = sev->target;
  ssev.property = sev->property;
  ssev.time = sev->time;

  XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

static void send_string(Display *dpy, XSelectionRequestEvent *sev, Atom atom,
                        const char *string, int length) {
  XSelectionEvent ssev;

  XChangeProperty(dpy, sev->requestor, sev->property, atom, 8, PropModeReplace,
                  (unsigned char *)string, length);

  ssev.type = SelectionNotify;
  ssev.requestor = sev->requestor;
  ssev.selection = sev->selection;
  ssev.target = sev->target;
  ssev.property = sev->property;
  ssev.time = sev->time;

  XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

static void send_uri_list(Display *dpy, XSelectionRequestEvent *sev,
                          Atom uri_list, const char *file) {
  char uri[strlen(file) + 7];
  sprintf(uri, "file://%s", file);
  send_string(dpy, sev, uri_list, uri, ARR_SIZE(uri));
}

static void send_gnome_copy(Display *dpy, XSelectionRequestEvent *sev,
                            Atom gnome_copy, const char *file) {
  char gnome_copy_uri[strlen(file) + 13];
  sprintf(gnome_copy_uri, "copy\nfile://%s", file);
  send_string(dpy, sev, gnome_copy, gnome_copy_uri, ARR_SIZE(gnome_copy_uri));
}

int copy_file(const char *file) {
  switch (fork()) {
  case -1: {
    return -1;
  }
  case 0: {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
      die("failed to open display %s\n", getenv("DISPLAY"));
    }

    Window root;
    {
      int screen = DefaultScreen(dpy);
      root = RootWindow(dpy, screen);
    }

    Window owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

    Atom atoms[XA_LAST];
    atoms[XA_TARGETS] = XInternAtom(dpy, "TARGETS", False);
    atoms[XA_URI_LIST] = XInternAtom(dpy, "text/uri-list", False);
    atoms[XA_GNOME_COPY] =
        XInternAtom(dpy, "x-special/gnome-copied-files", False);

    Atom selection = XInternAtom(dpy, "CLIPBOARD", False);
    XSetSelectionOwner(dpy, selection, owner, CurrentTime);
    XEvent ev;
    for (;;) {
      XNextEvent(dpy, &ev);
      switch (ev.type) {
      case SelectionClear:
        remove_file(file);
        XCloseDisplay(dpy);
        exit(EXIT_SUCCESS);
      case SelectionRequest: {
        XSelectionRequestEvent *sev =
            (XSelectionRequestEvent *)&ev.xselectionrequest;
        if (sev->property != None) {
          if (sev->target == atoms[XA_TARGETS]) {
            send_targets_list(dpy, sev, atoms[XA_TARGETS], atoms, XA_LAST);
            break;
          } else if (sev->target == atoms[XA_URI_LIST]) {
            send_uri_list(dpy, sev, atoms[XA_URI_LIST], file);
            break;
          } else if (sev->target == atoms[XA_GNOME_COPY]) {
            send_gnome_copy(dpy, sev, atoms[XA_GNOME_COPY], file);
            break;
          }
        }
        deny_selection_request(dpy, sev);
      }
      }
    }
  }
  default: {
    return 0;
  }
  }
}
