/*! # train.h

Contains the `wagon_t` and `train_t` structures.
*/

#ifndef TRAIN_H
#define TRAIN_H

#include "container.h"
#include <stdbool.h>

#define WAGON_CONTAINERS 2
#define TRAIN_WAGONS 10
#define LANE_WAGONS (2 * TRAIN_WAGONS)

struct train;

struct wagon {
    /// The cargo on the wagon
    container_holder_t containers[WAGON_CONTAINERS];

    /// Reference to the parent thread.
    /// It is unsafe to assume that if we own the wagon, we may then read/write from the train
    /// This is mainly to know which wagon belongs to which train
    const struct train* train;

    /// A copy of the train's destination, follows train_t's guarantees on destination
    size_t destination;

    /// Unique identifier for the wagon, see ulid.h for more information
    unsigned char ulid[16];
};
typedef struct wagon wagon_t;

struct train {
    /// The wagons making up the train; it is unsafe to access any of the wagons for which we aren't the owner
    wagon_t wagons[TRAIN_WAGONS];

    /// The number of wagons that the train has; guaranteed to be less than or equal to TRAIN_WAGONS
    size_t n_wagons;

    /// True if the wagon is full and ready to be sent
    bool wagon_full[TRAIN_WAGONS];

    /// Guaranteed to be a valid index of DESTINATION_NAMES
    size_t destination;
};
typedef struct train train_t;

/// Creates a new wagon instance; will read the destination from `train`.
wagon_t new_wagon(const train_t* train, size_t n_cargo);

/// Used for debugging
void print_wagon(wagon_t* wagon, bool newline);

/// Returns information about how loaded a wagon is
bool wagon_is_full(wagon_t* wagon);
size_t wagon_loaded(wagon_t* wagon);

train_t* new_train(size_t destination, size_t n_wagons);

void free_train(train_t* train);

struct train_lane {
    wagon_t* wagons[LANE_WAGONS];
    size_t n_wagons;

    pthread_mutex_t mutex;
};
typedef struct train_lane train_lane_t;

/// Creates a new train_lane, with no wagons
train_lane_t new_train_lane();

/// Frees a train lane; does not free the wagons in it
void free_train_lane(train_lane_t* train_lane);

/// Used for debugging
void train_lane_print(train_lane_t* train_lane, bool short_version);

/// Locks and unlocks the underlying mutex
void train_lane_lock(train_lane_t* train_lane);
void train_lane_unlock(train_lane_t* train_lane);

/// Shifts the wagons in the train lane by `shift_by` spots, removing the `shift_by` first wagons.
/// Does *not* lock the underlying mutex
void train_lane_shift(train_lane_t* train_lane, size_t shift_by);

/// Appends a wagon to the train lane
/// Does *not* lock the underlying mutex
void train_lane_append(train_lane_t* train_lane, wagon_t* wagon);

/// Finds and returns a wagon_t that can accept a container with destination `destination`;
/// If none are found, returns NULL
/// Does *not* lock the train lane (as the returned reference outlives the function's scope)
wagon_t* train_lane_accepts(train_lane_t* train_lane, size_t destination);

#endif // TRAIN_H
