/*! # control-tower.h

Contains functions related to the control tower.
Particularly, it contains the functions necessary to communicate with the control tower.

*/

#ifndef CONTROL_TOWER_H
#define CONTROL_TOWER_H

#include <pthread.h>
#include "container.h"
#include "boat.h"

enum message_type {
    BOAT_EMPTY,
    BOAT_FULL
};

union message_data {
    boat_t boat;
};

struct message {
    enum message_type type;
    union message_data data;

    struct message* next;
    unsigned char ulid[16];
};
typedef struct message message_t;

struct control_tower {
    message_t* message_queue;

    pthread_mutex_t message_mutex;
    pthread_cond_t message_monitor;
};
typedef struct control_tower control_tower_t;

/// Creates a new message
message_t* new_message(enum message_type type, union message_data data);

/// Prints a message, used for debugging
void print_message(message_t* message);

/// Creates a new control_tower, with an empty message queue
control_tower_t new_control_tower();

/// Frees a control_tower instance, must be called once for each instance
void free_control_tower(control_tower_t* control_tower);

/// Safely sends a message to the tower, locking the necessary mutexes.
/// `message` may not be accessed by the current thread after a call to this function
void control_tower_send(control_tower_t* tower, message_t* message);

/// Safely reads a message from the message queue of the tower, and sleeps if there are no message available.
message_t* control_tower_receive(control_tower_t* tower);

#endif // CONTROL_TOWER_H
