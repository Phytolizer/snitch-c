#pragma once

#include "span.h"

#include <stdarg.h>

typedef SPAN_TYPE(char) string_t;

#define STRING_C_INIT(c) SPAN_WITH_LENGTH(c, sizeof c - 1)
#define STRING_C(c) (string_t) STRING_C_INIT(c)
#define STRING_FMT ".*s"
#define STRING_ARG(s) (int)(s).length, (s).begin

string_t string_from_c(char* c);
string_t string_from_sprintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
string_t string_from_vsprintf(const char* fmt, va_list args);
char* string_to_c(string_t s);
