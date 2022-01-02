/*! # ulid.h

Contains methods for generating Universally Unique Lexicographically Sortable Identifiers (ULIDs).
This uses the `ulid-c` library: https://github.com/skeeto/ulid-c

This file maintains a thread-specific `ulid_generator`, which can be retrieved with `get_generator`.
Because this header includes ulid-c, you can then use this library's functions on the `ulid_generator`.
*/

#ifndef MY_ULID_H
#define MY_ULID_H

#include <ulid.h>

/**
Returns a thread-specific `ulid_generator`.
**/
struct ulid_generator* get_generator();

#endif // MY_ULID_H
