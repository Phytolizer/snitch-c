#pragma once

#include <stddef.h>

#define SPAN_TYPE(T) \
    struct { \
        T* begin; \
        T* end; \
        size_t length; \
    }

#define SPAN_FROM_BOUNDS(begin, end) \
    { (begin), (end), (end) - (begin) }

#define SPAN_WITH_LENGTH(begin, length) \
    { (begin), (begin) + (length), (length) }

#define SPAN_EMPTY \
    { NULL, NULL, 0 }

#define SPAN_FROM_ARRAY(array) SPAN_WITH_LENGTH(array, sizeof(array) / sizeof(array[0]))

typedef SPAN_TYPE(int) int_span_t;
typedef SPAN_TYPE(unsigned int) uint_span_t;
typedef SPAN_TYPE(float) float_span_t;
