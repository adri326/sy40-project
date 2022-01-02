/*! # message.h

Common class for `control_tower.h` and `crane.h`, which handles the messages.
*/

#ifndef MESSAGE_H
#define MESSAGE_H

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

/// Creates a new message
message_t* new_message(enum message_type type, union message_data data);

/// Prints a message, used for debugging
void print_message(message_t* message);

/// Should be called to free the message
void free_message(message_t* message);

#endif // MESSAGE_H
