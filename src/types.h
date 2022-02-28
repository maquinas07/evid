#ifndef EVID_TYPES_H
#define EVID_TYPES_H

typedef struct _Region _Region;
struct _Region {
  int x;          /* offset from left of screen */
  int y;          /* offset from top of screen */
  int X;          /* offset from right of screen */
  int Y;          /* offset from bottom of screen */
  unsigned int w; /* width */
  unsigned int h; /* height */
  unsigned int b; /* border_width */
  unsigned int d; /* depth */
};

typedef struct Audio Audio;
struct Audio {
  char *subsystem;
  char *input;
};

typedef struct Args Args;
struct Args {
  Audio *audio;
  int verbosity;
#ifdef HAVE_ZENITY
  int use_zenity;
#endif
  int draw_mouse;
  int show_region;
  int gif;
  char *framerate;
  char *output;
};

#endif
