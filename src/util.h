/**
    util.h - part of evid
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

#ifndef EVID_UTIL_H
#define EVID_UTIL_H

#include "evid.h"

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(*arr))

void error(const char *errstr, ...);
void die(const char *errstr, ...);

void print_version(void);
void print_usage(void);

#endif
