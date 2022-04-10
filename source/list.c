#include "snitch/list.h"

#include "snitch/todo.h"

void list_subcommand(void) {
    todo_span_t todos = todos_of_dir(STRING_C("."));
    for (todo_t* todo = todos.begin; todo != todos.end; todo++) {
        todo_print(todo, stdout);
    }
}
