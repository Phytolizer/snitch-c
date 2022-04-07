#pragma once

#include "span.h"
#include "string_span.h"

#include <stddef.h>
#include <stdio.h>

typedef struct {
    string_t prefix;
    string_t suffix;
    string_t id;
    string_t filename;
    size_t line;
} todo_t;

string_t todo_to_string(todo_t* todo);
void todo_print(todo_t* todo, FILE* stream);

typedef SPAN_TYPE(todo_t) todo_span_t;

todo_span_t todos_of_dir(string_t dir_path);
