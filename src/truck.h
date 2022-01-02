/*! # truck.h

Contains the truck_t type.

*/

#ifndef TRUCK_H
#define TRUCK_H

#include "container.h"
#include <stdbool.h>

struct truck {
    container_holder_t container;

    /// Guaranteed to be a valid DESTINATION_NAMES index
    size_t destination;

    bool loading;

    unsigned char ulid[16];
};
typedef struct truck truck_t;

/// Creates a new truck
truck_t new_truck(size_t destination);

/// Used for debugging
void print_truck(truck_t* truck, bool newline);

struct truck_ll {
    /// Exclusive ownership of the truck is guaranteed
    truck_t* truck;

    struct truck_ll* next;
};

struct truck_lane {
    struct truck_ll* trucks;
};
typedef struct truck_lane truck_lane_t;

/// Creates an empty truck lane
truck_lane_t new_truck_lane();

void free_truck_lane(truck_lane_t* truck_lane);

/// Adds a truck to the truck lane
void truck_lane_push(truck_lane_t* lane, truck_t* truck);

/// Prints the truck lane, used for debugging
void truck_lane_print(truck_lane_t* lane, bool short_version);

#endif // TRUCK_H
