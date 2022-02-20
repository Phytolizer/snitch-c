#include "string_span.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct string_span string_span_from_cstr(char* cstr) {
  return (struct string_span){cstr, strlen(cstr)};
}

struct string_span string_span_dup(char* str, size_t len) {
  struct string_span result = {.data = calloc(len + 1, 1), .length = len};
  memcpy(result.data, str, len);
  return result;
}

struct string_span string_span_trim_left(struct string_span ss) {
  size_t i = 0;

  while (i < ss.length && isspace(ss.data[i])) {
    i += 1;
  }

  return (struct string_span){ss.data + i, ss.length - i};
}

struct string_span string_span_trim_right(struct string_span ss) {
  size_t i = 0;

  while (i < ss.length && isspace(ss.data[ss.length - i - 1])) {
    i += 1;
  }

  return (struct string_span){ss.data, ss.length - i};
}

struct string_span string_span_trim(struct string_span ss) {
  return string_span_trim_right(string_span_trim_left(ss));
}

struct string_span string_span_chop_left(struct string_span* ss, size_t n) {
  if (n > ss->length) {
    n = ss->length;
  }

  struct string_span result = {ss->data, n};

  ss->data += n;

  ss->length -= n;

  return result;
}

struct string_span string_span_chop_right(struct string_span* ss, size_t n) {
  if (n > ss->length) {
    n = ss->length;
  }

  struct string_span result = {ss->data + ss->length - n, n};

  ss->length -= n;

  return result;
}

bool string_span_index_of(struct string_span ss, char c, size_t* index) {
  size_t i = 0;

  while (i < ss.length && ss.data[i] != c) {
    i += 1;
  }

  if (i < ss.length) {
    if (index != NULL) {
      *index = i;
    }

    return true;
  }

  return false;
}

bool string_span_try_chop_by_delim(struct string_span* ss, char delim,
                                   struct string_span* chunk) {
  size_t i = 0;

  while (i < ss->length && ss->data[i] != delim) {
    i += 1;
  }

  struct string_span result = {ss->data, i};

  if (i < ss->length) {
    ss->length -= i + 1;
    ss->data += i + 1;

    if (chunk != NULL) {
      *chunk = result;
    }

    return true;
  }

  return false;
}

struct string_span string_span_chop_by_delim(struct string_span* ss,
                                             char delim) {
  size_t i = 0;

  while (i < ss->length && ss->data[i] != delim) {
    i += 1;
  }

  struct string_span result = {ss->data, i};

  if (i < ss->length) {
    ss->length -= i + 1;
    ss->data += i + 1;
  } else {
    ss->length -= i;
    ss->data += i;
  }

  return result;
}

bool string_span_eq(struct string_span a, struct string_span b) {
  return a.length == b.length && memcmp(a.data, b.data, a.length) == 0;
}

bool string_span_starts_with(struct string_span ss,
                             struct string_span expected_prefix) {
  if (expected_prefix.length > ss.length) {
    return false;
  }

  struct string_span actual_prefix = {ss.data, expected_prefix.length};

  return string_span_eq(actual_prefix, expected_prefix);
}

bool string_span_ends_with(struct string_span ss,
                           struct string_span expected_suffix) {
  if (expected_suffix.length > ss.length) {
    return false;
  }

  struct string_span actual_suffix = {
      ss.data + ss.length - expected_suffix.length, expected_suffix.length};

  return string_span_eq(actual_suffix, expected_suffix);
}
