#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static char* string_dup(const char* str) {
  size_t n = strlen(str);

  char* result = calloc(n + 1, 1);
  assert(result != NULL && "Memory allocation failure");

  memcpy(result, str, n);
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
  const todo_t* data;
  size_t length;
} todos_t;

static char* todo_to_string(todo_t t) {
  if (t.id == NULL) {
    return allocated_sprintf("%s:%zu: %sTODO: %s\n", t.filename, t.line,
                             t.prefix, t.suffix);
  }

  return allocated_sprintf("%s:%zu: %sTODO(%s): %s\n", t.filename, t.line,
                           t.prefix, t.id, t.suffix);
}

static todos_t todos_of_dir(const char* dir_path) {
  static const todo_t todos[] = {
      {"// ", "khooy", "42", "./main.go", 10},
      {"// ", "foo", NULL, "./src/foo.go", 0},
  };
  return (todos_t){
      todos,
      sizeof todos / sizeof(todo_t),
  };
}

static void list_subcommand(void) {
  todos_t todos = todos_of_dir(".");
  for (size_t i = 0; i < todos.length; i += 1) {
    char* todo = todo_to_string(todos.data[i]);
    printf("%s", todo);
    free(todo);
  }
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
