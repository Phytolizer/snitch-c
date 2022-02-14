#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "string_span.h"
#include "walk.h"

static char* allocated_sprintf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int n = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (n <= 0) {
    return NULL;
  }

  char* result = calloc(n + 1, 1);
  assert(result != NULL && "Memory allocation failure");

  va_start(args, format);
  vsnprintf(result, n + 1, format, args);
  va_end(args);

  return result;
}

typedef struct {
  char* prefix;
  char* suffix;
  char* id;
  char* filename;
  size_t line;
} todo_t;

typedef struct {
  todo_t* data;
  size_t length;
  size_t capacity;
} todos_t;

static void todos_push(todos_t* todos, todo_t todo) {
  if (todos->length == todos->capacity) {
    todos->capacity = todos->capacity * 2 + 1;
    todo_t* new_todos = calloc(todos->capacity, sizeof(todo_t));
    if (todos->length > 0) {
      memcpy(new_todos, todos->data, todos->length * sizeof(todo_t));
    }
    free(todos->data);
    todos->data = new_todos;
  }

  todos->data[todos->length] = todo;
  todos->length += 1;
}

static char* todo_to_string(todo_t t) {
  if (t.id == NULL) {
    return allocated_sprintf("%s:%zu: %sTODO: %s\n", t.filename, t.line,
                             t.prefix, t.suffix);
  }

  return allocated_sprintf("%s:%zu: %sTODO(%s): %s\n", t.filename, t.line,
                           t.prefix, t.id, t.suffix);
}

typedef struct {
  bool success;
  todos_t todos;
} maybe_todos_t;

static todo_t* line_as_todo(struct string_span line) {
  (void)line;
  return NULL;
}

static maybe_todos_t todos_of_file(const char* file_path) {
  FILE* fp = fopen(file_path, "r");
  if (fp == NULL) {
    return (maybe_todos_t){.success = false};
  }

  todos_t todos = {0};
  char* line = NULL;
  size_t line_alloc_length = 0;
  ssize_t line_length;
  while ((line_length = getline(&line, &line_alloc_length, fp)) != -1) {
    todo_t* todo = line_as_todo((struct string_span){line, line_length});
    if (todo != NULL) {
      todos_push(&todos, *todo);
    }
    free(todo);
  }
  free(line);
  return (maybe_todos_t){.success = true, .todos = todos};
}

static maybe_todos_t todos_of_dir(const char* dir_path) {
  struct walkdir walk = walkdir_open(dir_path);
  todos_t all_todos = {0};
  while (true) {
    struct dirent* entry = walkdir_next(&walk);
    if (entry == NULL) {
      break;
    }
    char* file_path = join_path(walkdir_dir_name(&walk), entry->d_name);
    maybe_todos_t file_todos = todos_of_file(file_path);
    if (!file_todos.success) {
      return file_todos;
    }
    todos_t todos = file_todos.todos;
    for (size_t i = 0; i < todos.length; i += 1) {
      todos_push(&all_todos, todos.data[i]);
    }
    free(todos.data);
    free(file_path);
  }
  if (errno != 0) {
    char errbuf[64];
    strerror_r(errno, errbuf, sizeof errbuf);
    fprintf(stderr, "%s\n", errbuf);
    exit(1);
  }
  walkdir_close(&walk);
  return (maybe_todos_t){.success = true, .todos = all_todos};
}

static void list_subcommand(void) {
  maybe_todos_t dir_todos = todos_of_dir(".");
  todos_t todos = dir_todos.todos;
  for (size_t i = 0; i < todos.length; i += 1) {
    char* todo = todo_to_string(todos.data[i]);
    printf("%s", todo);
    free(todo);
  }
  free(todos.data);
}

static void report_subcommand(void) {
  fprintf(stderr, "report is not implemented");
  exit(1);
}

int main(int argc, char** argv) {
  if (strcmp(argv[1], "list") == 0) {
    list_subcommand();
  } else if (strcmp(argv[1], "report") == 0) {
    report_subcommand();
  } else {
    fprintf(stderr, "`%s` unknown command\n", argv[1]);
    return 1;
  }
}
