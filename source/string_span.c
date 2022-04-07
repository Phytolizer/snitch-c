#include "string_span.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

string_t string_from_c(char* c) {
    return (string_t)SPAN_WITH_LENGTH(c, strlen(c));
}

string_t string_from_sprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    string_t s = string_from_vsprintf(fmt, args);
    va_end(args);
    return s;
}

string_t string_from_vsprintf(const char* fmt, va_list args) {
    va_list args2;
    va_copy(args2, args);
    int len = vsnprintf(NULL, 0, fmt, args2);
    va_end(args2);
    if (len < 0) {
        return (string_t)SPAN_EMPTY;
    }
    char* buf = malloc(len + 1);
    vsnprintf(buf, len + 1, fmt, args);
    return (string_t)SPAN_WITH_LENGTH(buf, len);
}
