/*! # container.h

Defines the `container_t` struct and its methods.
*/

#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#define N_DESTINATIONS 5
static const char* DESTINATION_NAMES[N_DESTINATIONS] = {
    "Paris",
    "Suez",
    "Bordeaux",
    "New York",
    "Singapour"
};

struct container {
    /// The index of the destination, guaranteed to be a valid index of DESINATION_NAMES
    size_t destination;
    /// A printable Universally Unique Lexicographically Sortable Identifier (ULID), unique to the container
    unsigned char ulid[16];
};
typedef struct container container_t;

/// Creates a new container; a thread-specific ulid_generator is implicitely created
container_t new_container(size_t destination);

/// Used for debugging
void print_container(container_t* container, bool newline);

/// Used by vehicles
struct container_holder {
    container_t container;
    bool is_empty;
};
typedef struct container_holder container_holder_t;

/// Creates a new container holder. If `is_empty` is true, all of the fields of the container hold are set to zero.
container_holder_t new_container_holder(bool is_empty, size_t destination);

/// Used for debugging
void print_container_holder(container_holder_t* holder, bool newline);

#endif // CONTAINER_H
