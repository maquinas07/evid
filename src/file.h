/**
    file.h - part of evid
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

#ifndef EVID_FILE_H
#define EVID_FILE_H
#include "types.h"
#include <stddef.h>

void get_default_file_name(char *default_file_name, Args *args);

int get_tmp_file(char *tmp_file, size_t tmp_file_size, Args *args);
int get_output_file(char *new_file, size_t new_file_size, Args *args);

void move_file(const char *source_file, const char *dest_file);
int remove_file(const char *file);
int mkdirp(const char *dir);

#ifdef HAVE_NOTIFY
int show_file_in_default_file_manager(const char *file);
#endif

#endif
