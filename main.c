#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pcre8.h"
#include "string_span.h"
#include "walk.h"

#define MAYBE_T(T) \
  struct {         \
    bool success;  \
    T value;       \
  }

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

typedef struct {
  int nothing;
} github_credentials_t;

typedef MAYBE_T(github_credentials_t) maybe_github_credentials_t;

maybe_github_credentials_t github_credentials_from_file(const char* filepath) {
  (void)filepath;
  return (maybe_github_credentials_t){.success = true, .value = {0}};
}

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
    return allocated_sprintf("%s:%zu: %sTODO: %s", t.filename, t.line, t.prefix,
                             t.suffix);
  }

  return allocated_sprintf("%s:%zu: %sTODO(%s): %s", t.filename, t.line,
                           t.prefix, t.id, t.suffix);
}

static bool todo_update(todo_t todo) {
  (void)todo;
  return true;
}

static pcre2_code* regex_must_compile(const char* regex) {
  int errcode;
  PCRE2_SIZE errofs;
  pcre2_code* code = pcre2_compile((PCRE2_SPTR)regex, PCRE2_ZERO_TERMINATED, 0,
                                   &errcode, &errofs, NULL);
  if (code == NULL) {
    PCRE2_UCHAR errbuf[128];
    pcre2_get_error_message(errcode, errbuf, sizeof errbuf);
    fprintf(stderr, "Compiling TODO pattern: %s\n", errbuf);
    return NULL;
  }
  return code;
}

static todo_t* line_as_unreported_todo(struct string_span line) {
  pcre2_code* unreported_todo_pattern = regex_must_compile("^(.*)TODO: (.*)$");
  if (unreported_todo_pattern == NULL) {
    return NULL;
  }
  pcre2_match_data* match_data =
      pcre2_match_data_create_from_pattern(unreported_todo_pattern, NULL);
  int errcode = pcre2_match(unreported_todo_pattern, (PCRE2_SPTR)line.data,
                            line.length, 0, 0, match_data, NULL);
  if (errcode < 0) {
    if (errcode != PCRE2_ERROR_NOMATCH) {
      PCRE2_UCHAR errbuf[128];
      pcre2_get_error_message(errcode, errbuf, sizeof errbuf);
      fprintf(stderr, "Matching TODO pattern: %s\n", errbuf);
    }
    return NULL;
  }
  PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
  char* prefix = calloc(ovector[3] - ovector[2] + 1, 1);
  memcpy(prefix, line.data + ovector[2], ovector[3] - ovector[2]);
  char* suffix = calloc(ovector[5] - ovector[4] + 1, 1);
  memcpy(prefix, line.data + ovector[4], ovector[5] - ovector[4]);
  pcre2_match_data_free(match_data);
  pcre2_code_free(unreported_todo_pattern);
  todo_t* todo = calloc(1, sizeof(todo_t));
  todo->prefix = prefix;
  todo->suffix = suffix;
  todo->filename = "";
  return todo;
}

static todo_t* line_as_reported_todo(struct string_span line) {
  // TODO(#2): line_as_todo has false positives inside binary files and string
  // literals.
  pcre2_code* reported_todo_pattern =
      regex_must_compile("^(.*)TODO\\((.*)\\): (.*)$");
  if (reported_todo_pattern == NULL) {
    return NULL;
  }
  pcre2_match_data* match_data =
      pcre2_match_data_create_from_pattern(reported_todo_pattern, NULL);
  int errcode = pcre2_match(reported_todo_pattern, (PCRE2_SPTR)line.data,
                            line.length, 0, 0, match_data, NULL);
  if (errcode < 0) {
    if (errcode != PCRE2_ERROR_NOMATCH) {
      PCRE2_UCHAR errbuf[128];
      pcre2_get_error_message(errcode, errbuf, sizeof errbuf);
      fprintf(stderr, "Matching TODO pattern: %s\n", errbuf);
    }
    return NULL;
  }
  PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
  char* prefix = calloc(ovector[3] - ovector[2] + 1, 1);
  memcpy(prefix, line.data + ovector[2], ovector[3] - ovector[2]);
  char* id = calloc(ovector[5] - ovector[4] + 1, 1);
  memcpy(id, line.data + ovector[4], ovector[5] - ovector[4]);
  char* suffix = calloc(ovector[7] - ovector[6] + 1, 1);
  memcpy(suffix, line.data + ovector[6], ovector[7] - ovector[6]);
  pcre2_match_data_free(match_data);
  pcre2_code_free(reported_todo_pattern);
  todo_t* todo = calloc(1, sizeof(todo_t));
  todo->prefix = prefix;
  todo->suffix = suffix;
  todo->filename = "";
  todo->id = id;
  return todo;
}

todo_t* line_as_todo(struct string_span line) {
  todo_t* todo = line_as_unreported_todo(line);
  if (todo != NULL) {
    return todo;
  }

  todo = line_as_reported_todo(line);
  if (todo != NULL) {
    return todo;
  }

  return NULL;
}

typedef bool todo_visitor_t(todo_t* todo, void* state);

static bool walk_todos_of_file(const char* file_path, todo_visitor_t visit,
                               void* visit_state) {
  FILE* fp = fopen(file_path, "r");
  if (fp == NULL) {
    return false;
  }

  char* line = NULL;
  size_t line_alloc_length = 0;
  ssize_t line_length;
  while ((line_length = getline(&line, &line_alloc_length, fp)) != -1) {
    todo_t* todo = line_as_todo((struct string_span){line, line_length});
    if (todo != NULL) {
      if (!visit(todo, visit_state)) {
        free(todo);
        return false;
      }
    }
    free(todo);
  }
  free(line);
  return true;
}

static bool walk_todos_of_dir(const char* dir_path, todo_visitor_t visit,
                              void* visit_state) {
  struct walkdir walk = walkdir_open(dir_path);
  todos_t all_todos = {0};
  while (true) {
    struct dirent* entry = walkdir_next(&walk);
    if (entry == NULL) {
      break;
    }
    char* file_path = join_path(walkdir_dir_name(&walk), entry->d_name);
    todos_t file_todos = {0};
    if (!walk_todos_of_file(file_path, visit, visit_state)) {
      return false;
    }
    for (size_t i = 0; i < file_todos.length; i += 1) {
      todos_push(&all_todos, file_todos.data[i]);
    }
    free(file_todos.data);
    free(file_path);
  }
  if (errno != 0) {
    char errbuf[64];
    strerror_r(errno, errbuf, sizeof errbuf);
    fprintf(stderr, "%s\n", errbuf);
    exit(1);
  }
  walkdir_close(&walk);
  return true;
}

static bool visit_todo(todo_t* todo, void* state) {
  (void)state;
  char* todo_str = todo_to_string(*todo);
  printf("%s\n", todo_str);
  free(todo_str);
  return true;
}

static bool list_subcommand(void) {
  // TODO(#3): list_subcommand doesn't handle a possible error case here.
  return walk_todos_of_dir(".", visit_todo, NULL);
}

typedef MAYBE_T(todo_t) maybe_todo_t;

static maybe_todo_t report_todo(todo_t todo, github_credentials_t creds) {
  (void)creds;
  return (maybe_todo_t){.success = true, .value = todo};
}

typedef struct {
  todos_t* reported_todos;
  github_credentials_t creds;
} report_subcommand_state_t;

static bool visit_report_todo(todo_t* todo, void* state) {
  if (todo->id != NULL) {
    report_subcommand_state_t* s = state;
    maybe_todo_t reported_todo = report_todo(*todo, s->creds);
    if (!reported_todo.success) {
      return false;
    }

    char* todo_text = todo_to_string(reported_todo.value);
    printf("[REPORTED] %s\n", todo_text);
    free(todo_text);

    todos_push(s->reported_todos, reported_todo.value);
  }
  return true;
}

static bool report_subcommand(github_credentials_t creds) {
  todos_t reported_todos = {0};
  report_subcommand_state_t state = {
      .reported_todos = &reported_todos,
      .creds = creds,
  };

  if (!walk_todos_of_dir(".", visit_report_todo, &state)) {
    return false;
  }

  for (size_t i = 0; i < reported_todos.length; i += 1) {
    if (!todo_update(reported_todos.data[i])) {
      return false;
    }
  }

  return true;
}

int main(int argc, char** argv) {
  maybe_github_credentials_t creds =
      github_credentials_from_file("~/.config/snitch/github.ini");
  if (!creds.success) {
    fprintf(stderr, "could not get github credentials\n");
    return 1;
  }

  if (argc > 1) {
    if (strcmp(argv[1], "list") == 0) {
      if (!list_subcommand()) {
        return 1;
      }
    } else if (strcmp(argv[1], "report") == 0) {
      report_subcommand(creds.value);
    } else {
      fprintf(stderr, "`%s` unknown command\n", argv[1]);
      return 1;
    }
  } else {
    // TODO(#5): implement a structure for these option descriptions
    fprintf(
        stderr,
        "snitch [opt]\n\tlist: list all todos of a dir recursively\n\treport: "
        "report an issue to github\n");
    return 1;
  }
}
