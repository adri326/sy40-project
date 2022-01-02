/*! # crane.h

Message-passing to/from crane threads is handled in this file.
Message-passing to the control tower is handled in `control_tower.h` and the messages
themselves in `message.h`

*/

#ifndef CRANE_H
#define CRANE_H

#include "message.h"
#include "boat.h"
#include "train.h"
#include <pthread.h>

struct crane {
    message_t* message_queue;
    pthread_mutex_t message_mutex;

    boat_lane_t boat_lane;
    train_lane_t train_lane;

    pthread_t thread;
};
typedef struct crane crane_t;

/// Creates a new crane_t instance
crane_t new_crane();

/// Should be called once for each crane_t instance
void free_crane(crane_t* crane);

/// Safely sends a message to the crane, locking and unlocking the necessary mutexes.
void crane_send(crane_t* crane, message_t* message);

/// Safely reads a message from the crane's queue, if there any.
/// Otherwise, returns NULL
message_t* crane_receive(crane_t* crane);

#endif // CRANE_H
