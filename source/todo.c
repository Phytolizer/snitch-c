#include "snitch/todo.h"

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

static todo_span_t todos_of_file(string_t path);

string_t todo_to_string(todo_t* todo) {
    if (todo->id.length == 0) {
        return string_from_sprintf("%" STRING_FMT ":%zu: %" STRING_FMT "TODO: %" STRING_FMT,
                STRING_ARG(todo->filename), todo->line, STRING_ARG(todo->prefix),
                STRING_ARG(todo->suffix));
    } else {
        return string_from_sprintf("%" STRING_FMT ":%zu: %" STRING_FMT "TODO(%" STRING_FMT
                                   "): %" STRING_FMT,
                STRING_ARG(todo->filename), todo->line, STRING_ARG(todo->prefix),
                STRING_ARG(todo->id), STRING_ARG(todo->suffix));
    }
}

void todo_print(todo_t* todo, FILE* stream) {
    string_t s = todo_to_string(todo);
    fprintf(stream, "%s\n", s.begin);
    free(s.begin);
}

todo_span_t todos_of_dir(string_t dir_path) {
    char* dir_path_c = string_to_c(dir_path);
    struct stat statbuf;
    if (stat(dir_path_c, &statbuf) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "%" STRING_FMT " is not a directory\n", STRING_ARG(dir_path));
        exit(EXIT_FAILURE);
    }

    DIR* dir = opendir(dir_path_c);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    todo_buffer_t todos = BUFFER_INIT;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        string_t path =
                string_from_sprintf("%" STRING_FMT "/%s", STRING_ARG(dir_path), entry->d_name);
        todo_span_t file_todos = todos_of_file(path);
        for (todo_t* todo = file_todos.begin; todo != file_todos.end; ++todo) {
            BUFFER_PUSH(&todos, *todo);
        }
        free(path.begin);
    }

    closedir(dir);
    free(dir_path_c);
    return (todo_span_t)BUFFER_AS_SPAN(todos);
}

inline todo_span_t todos_of_file(string_t path) {
    // TODO: todos_of_file is not implemented
    (void)path;
    static todo_t test[] = {
            {
                    STRING_C_INIT("// "),
                    STRING_C_INIT("khooy"),
                    STRING_C_INIT("#42"),
                    STRING_C_INIT("./source/main.c"),
                    10,
            },
            {
                    STRING_C_INIT("// "),
                    STRING_C_INIT("foo"),
                    SPAN_EMPTY,
                    STRING_C_INIT("./src/foo.c"),
                    0,
            },
    };

    return (todo_span_t)SPAN_FROM_ARRAY(test);
}
