#include "walk.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#define PATH_SEP_LEN (sizeof PATH_SEP - 1)

char* join_path(const char* parent, const char* child) {
  size_t parent_len = strlen(parent);
  size_t child_len = strlen(child);

  char* result = calloc(parent_len + PATH_SEP_LEN + child_len + 1, 1);
  memcpy(result, parent, parent_len);
  memcpy(result + parent_len, PATH_SEP, PATH_SEP_LEN);
  memcpy(result + parent_len + PATH_SEP_LEN, child, child_len);
  return result;
}

static void walkdir_push(struct walkdir* walk, DIR* dir, const char* path) {
  if (walk->stack_top == walk->stack_cap) {
    walk->stack_cap = walk->stack_cap * 2 + 1;
    DIR** new_stack = calloc(walk->stack_cap, sizeof(DIR*));
    if (walk->stack_top > 0) {
      memcpy(new_stack, walk->stack, walk->stack_top * sizeof(DIR*));
    }
    free(walk->stack);
    walk->stack = new_stack;
    char** new_dir_names = calloc(walk->stack_cap, sizeof(char*));
    if (walk->stack_top > 0) {
      memcpy(new_dir_names, walk->dir_names, walk->stack_top * sizeof(char*));
    }
    free(walk->dir_names);
    walk->dir_names = new_dir_names;
  }

  walk->stack[walk->stack_top] = dir;
  size_t path_len = strlen(path);
  walk->dir_names[walk->stack_top] = calloc(path_len + 1, 1);
  memcpy(walk->dir_names[walk->stack_top], path, path_len);
  walk->stack_top += 1;
}

static DIR* walkdir_top(struct walkdir* walk) {
  if (walk->stack_top == 0) {
    return NULL;
  }
  return walk->stack[walk->stack_top - 1];
}

char* walkdir_dir_name(struct walkdir* walk) {
  if (walk->stack_top == 0) {
    return NULL;
  }
  return walk->dir_names[walk->stack_top - 1];
}

static DIR* walkdir_pop(struct walkdir* walk) {
  DIR* result = walkdir_top(walk);
  if (result == NULL) {
    return NULL;
  }
  walk->stack_top -= 1;
  return result;
}

struct walkdir walkdir_open(const char* path) {
  struct walkdir walk = {
      .stack = NULL,
      .stack_top = 0,
      .stack_cap = 0,
  };

  DIR* d = opendir(path);
  walkdir_push(&walk, d, path);

  return walk;
}

struct dirent* walkdir_next(struct walkdir* walk) {
  while (walk->stack_top > 0) {
    errno = 0;
    struct dirent* entry = readdir(walkdir_top(walk));
    if (entry != NULL) {
      if (entry->d_type == DT_DIR) {
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
          char* child_path = join_path(walkdir_dir_name(walk), entry->d_name);
          walkdir_push(walk, opendir(child_path), child_path);
          free(child_path);
        }
      } else if (entry->d_type == DT_REG) {
        return entry;
      }
    } else if (errno != 0) {
      return NULL;
    } else {
      free(walkdir_dir_name(walk));
      walkdir_pop(walk);
    }
  }

  return NULL;
}

void walkdir_close(struct walkdir* walkdir) {
  free(walkdir->stack);
  free(walkdir->dir_names);
  walkdir->stack = NULL;
  walkdir->stack_cap = 0;
  walkdir->stack_top = 0;
}
