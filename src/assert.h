#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

/// Various assertion functions, which are all based around `passert_op`
/// These check for `errno` too

#ifndef ASSERT_H
#define ASSERT_H

#define FMT_ERROR(str) str
#define FMT_WARN(str) str
#define FMT_INFO(str) str

// Compatibility mode removes the VA_OPT option and the ANSI colors
#ifdef COMPAT
    #define eprintf_va(...) /* noop */
#else
    #ifdef linux
        #undef FMT_ERROR
        #define FMT_ERROR(str) "\x1b[31m" str "\x1b[0m"
        #undef FMT_WARN
        #define FMT_WARN(str) "\x1b[93m" str "\x1b[0m"
        #undef FMT_INFO
        #define FMT_INFO(str) "\x1b[96m" str "\x1b[0m"
    #endif

    #define eprintf_va(...) __VA_OPT__(fprintf(stderr, FMT_INFO("INFO") ": " __VA_ARGS__); fprintf(stderr, "\n");)
#endif

#define passert_op(type, print, left, right, op, op_neg, ...) { int line = __LINE__; \
    errno = 0; \
    type l = left; \
    type r = right; \
    if (l op_neg r) { \
        if (errno != 0) { \
            char buffer[1024]; \
            sprintf(buffer, FMT_WARN("WARN") ": " __FILE__ ":%d errno is non-zero", line); \
            perror(buffer); \
        } \
        fprintf(stderr, FMT_ERROR("ERROR") ": " __FILE__ ":%d Assertion failed: " print " " #op_neg " " print "\n", line, l, r); \
        fprintf(stderr, FMT_ERROR("ERROR") ": " __FILE__ ":%d Expected " #left " " #op " " #right "\n", line); \
        eprintf_va(__VA_ARGS__); \
        exit(1); \
    } \
}

#define passert(value, ...) { int line = __LINE__; \
    errno = 0; \
    int x = (int)(value); \
    if (x == 0) { \
        if (errno != 0) { \
            char buffer[1024]; \
            sprintf(buffer, FMT_WARN("WARN") ": " __FILE__ ":%d errno is non-zero", line); \
            perror(buffer); \
        } \
        fprintf(stderr, FMT_ERROR("ERROR") ": " __FILE__ ":%d Assertion failed: " #value " returned %d\n", line, x); \
        fprintf(stderr, FMT_ERROR("ERROR") ": " __FILE__ ":%d Expected " #value " to be true (nonzero).\n", line); \
        eprintf_va(__VA_ARGS__); \
        exit(1); \
    } \
}

#define passert_eq(type, print, left, right, ...) passert_op(type, print, left, right, ==, !=, __VA_ARGS__)

#define passert_neq(type, print, left, right, ...) passert_op(type, print, left, right, !=, ==, __VA_ARGS__)

#define passert_gt(type, print, left, right, ...) passert_op(type, print, left, right, >, <=, __VA_ARGS__)

#define passert_lt(type, print, left, right, ...) passert_op(type, print, left, right, <, >=, __VA_ARGS__)

#define passert_gte(type, print, left, right, ...) passert_op(type, print, left, right, >=, <, __VA_ARGS__)

#define passert_lte(type, print, left, right, ...) passert_op(type, print, left, right, <=, >, __VA_ARGS__)

#endif // ASSERT_H
