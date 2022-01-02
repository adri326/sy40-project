/*! # boat.h

Contains the `boat_t` struct, alongside a vecdeque of boats and the mutex system for a boat lane.

*/

#ifndef BOAT_H
#define BOAT_H

#include "container.h"
#include <stdlib.h>
#include <stdbool.h>

#define BOAT_CONTAINERS 5

struct boat {
    // The cargo of that boat
    container_holder_t containers[BOAT_CONTAINERS];
    // Guaranteed to be a valid index of DESTINATION_NAMES
    size_t destination;
    // The unique ID of the boat, see container.h for more information
    unsigned char ulid[16];
};
typedef struct boat boat_t;

/// Creates a new boat, giving it a destination and n_cargo containers with random destinations.
/// Their destinations will be different than the boat's destination, if possible
boat_t new_boat(size_t destination, size_t n_cargo);

/// Prints a boat, used for debugging.
void print_boat(const boat_t* boat, bool newline);
#endif // BOAT_H
