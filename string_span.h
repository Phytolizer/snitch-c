#ifndef STRING_VIEW_H_
#define STRING_VIEW_H_

#include <stdbool.h>
#include <stddef.h>

struct string_span {
  char* data;
  size_t length;
};

struct string_span string_span_from_cstr(char* cstr);
struct string_span string_span_dup(char* str, size_t len);
struct string_span string_span_trim_left(struct string_span ss);
struct string_span string_span_trim_right(struct string_span ss);
struct string_span string_span_trim(struct string_span ss);
struct string_span string_span_chop_left(struct string_span* ss, size_t n);
struct string_span string_span_chop_right(struct string_span* ss, size_t n);
bool string_span_index_of(struct string_span ss, char c, size_t* index);
bool string_span_try_chop_by_delim(struct string_span* ss, char delim,
                                   struct string_span* chunk);
struct string_span string_span_chop_by_delim(struct string_span* ss,
                                             char delim);
bool string_span_eq(struct string_span a, struct string_span b);
bool string_span_starts_with(struct string_span ss,
                             struct string_span expected_prefix);
bool string_span_ends_with(struct string_span ss,
                           struct string_span expected_suffix);

#define STRING_SPAN_NULL ((struct string_span){NULL, 0})
#define STRING_SPAN_ARG(Span) (int)(Span).length, (Span).data
#define STRING_SPAN_C(Cstr) ((struct string_span){Cstr, sizeof(Cstr) - 1})

#endif
