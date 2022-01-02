/*! # control-tower.h

Contains functions related to the control tower.
Particularly, it contains the functions necessary to communicate with the control tower.

*/

#ifndef CONTROL_TOWER_H
#define CONTROL_TOWER_H

struct control_tower;

#include <pthread.h>
#include "container.h"
#include "message.h"
#include "boat.h"
#include "crane.h"

struct control_tower {
    message_t* message_queue;

    pthread_mutex_t message_mutex;
    pthread_cond_t message_monitor;

    struct crane* crane_alpha;
    struct crane* crane_beta;

    pthread_t thread;
};
typedef struct control_tower control_tower_t;

/// Creates a new control_tower, with an empty message queue
control_tower_t new_control_tower();

/// Frees a control_tower instance, must be called once for each instance
void free_control_tower(control_tower_t* control_tower);

/// Safely sends a message to the tower, locking the necessary mutexes.
/// `message` may not be accessed by the current thread after a call to this function
void control_tower_send(control_tower_t* tower, message_t* message);

/// Safely reads a message from the message queue of the tower, and sleeps if there are no message available.
message_t* control_tower_receive(control_tower_t* tower);

void* control_tower_entry(void* data);

#endif // CONTROL_TOWER_H
