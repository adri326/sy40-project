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

bool boat_is_full(boat_t* boat);
size_t boat_loaded(boat_t* boat);
container_holder_t* boat_first_empty(boat_t* boat);

/// Boat double-ended queue (DEQue)
struct boat_deque {
    boat_t* buffer;
    size_t capacity;
    size_t begin;
    size_t length;
};
typedef struct boat_deque boat_deque;

/// Creates a new, empty boat_deque instance, with `capacity` capacity
/// `capacity` must be greater than zero (or else UB might happen)
boat_deque* new_boat_deque(size_t capacity);

/// Frees a boat_deque instance. Must be called, or else memory will be leaked
void free_boat_deque(boat_deque* queue);

/// Reallocates the buffer of `queue` to be of capacity `capacity`
void boat_deque_resize(boat_deque* queue, size_t capacity);

/// Pops a boat at the head of the queue; if the queue is empty, returns false.
bool boat_deque_pop_front(boat_deque* queue, boat_t* dest);

/// Returns a reference to the n-th element from the head of the queue.
/// This reference is only valid until queue is manipulated.
/// If the queue is empty, returns NULL.
boat_t* boat_deque_get(boat_deque* queue, size_t n);

/// Pushes a boat onto the queue. If the queue is full, reallocates a new buffer
void boat_deque_push_back(boat_deque* queue, boat_t boat);

/// Prints a boat queue, used for debugging
void boat_deque_print(boat_deque* queue, bool short_version);

struct boat_lane {
    boat_deque* queue;
    boat_t current_boat;
    bool has_current_boat;

    pthread_mutex_t mutex;
};
typedef struct boat_lane boat_lane_t;

/// Creates a new boat_lane, with an empty queue and no stationned boat
boat_lane_t new_boat_lane();

/// Should be called once for every boat_lane_t instance
void free_boat_lane(boat_lane_t* boat_lane);

/// Used for debugging, does *not* lock the boat lane
void boat_lane_print(boat_lane_t* boat_lane, bool short_version);

/// Locks and unlocks the mutex of the boat_lane_t instance
void boat_lane_lock(boat_lane_t* boat_lane);
void boat_lane_unlock(boat_lane_t* boat_lane);

/// Finds and returns a boat_t that can accept a container with destination `destination`;
/// If none are found, returns NULL
/// Does *not* lock the boat lane (as the returned reference outlives the function's scope)
boat_t* boat_lane_accepts(boat_lane_t* boat_lane, size_t destination);

#endif // BOAT_H
