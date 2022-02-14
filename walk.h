#pragma once

#include <dirent.h>
#include <stddef.h>

struct walkdir {
  DIR** stack;
  char** dir_names;
  size_t stack_top;
  size_t stack_cap;
};

char* join_path(const char* parent, const char* child);
char* walkdir_dir_name(struct walkdir* walk);
struct walkdir walkdir_open(const char* path);
struct dirent* walkdir_next(struct walkdir* walkdir);
void walkdir_close(struct walkdir* walkdir);
