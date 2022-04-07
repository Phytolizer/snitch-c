#include "snitch/todo.h"

#include <stdlib.h>

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
    (void)dir_path;
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
