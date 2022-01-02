/*! # crane.h

Message-passing to/from crane threads is handled in this file.
Message-passing to the control tower is handled in `control_tower.h` and the messages
themselves in `message.h`

*/

#ifndef CRANE_H
#define CRANE_H

struct crane;

#include "message.h"
#include "boat.h"
#include "train.h"
#include "truck.h"
#include "control_tower.h"
#include <pthread.h>

struct crane {
    message_t* message_queue;
    pthread_mutex_t message_mutex;

    boat_lane_t boat_lane;
    bool load_boats;

    train_lane_t train_lane;
    bool load_trains;

    truck_lane_t truck_lane;

    struct control_tower* control_tower;

    pthread_t thread;
};
typedef struct crane crane_t;

/// Creates a new crane_t instance
crane_t new_crane(bool load_boats, bool load_trains);

/// Should be called once for each crane_t instance
void free_crane(crane_t* crane);

/// Used for debugging
void print_crane(crane_t* crane);

/// Safely sends a message to the crane, locking and unlocking the necessary mutexes.
void crane_send(crane_t* crane, message_t* message);

/// Safely reads a message from the crane's queue, if there any.
/// Otherwise, returns NULL
message_t* crane_receive(crane_t* crane);

void* crane_entry(void* data);

#endif // CRANE_H
