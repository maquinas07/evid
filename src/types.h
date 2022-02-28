/**
    types.h - part of evid
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
