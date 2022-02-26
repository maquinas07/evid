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

#endif
