/*! # message.h

Common class for `control_tower.h` and `crane.h`, which handles the messages.
*/

#ifndef MESSAGE_H
#define MESSAGE_H

#include "boat.h"
#include "truck.h"
#include "train.h"
#include <pthread.h>

enum message_type {
    BOAT_EMPTY,
    BOAT_FULL,
    TRUCK_FULL,
    TRUCK_EMPTY,
    TRUCK_NEW,
    WAGON_FULL,
    WAGON_EMPTY,
    CRANE_STUCK
};

union message_data {
    boat_t boat;
    truck_t* truck;
    wagon_t* wagon; // NOTE: this value of wagon may not be dereferenced
    bool stuck;
};

struct message {
    enum message_type type;
    union message_data data;

    struct message* next;
    unsigned char ulid[16];

    pthread_t sender;
};
typedef struct message message_t;

/// Creates a new message
message_t* new_message(enum message_type type, union message_data data);

/// Prints a message, used for debugging
void print_message(message_t* message);

/// Should be called to free the message
void free_message(message_t* message);

#endif // MESSAGE_H
