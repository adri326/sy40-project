#include "message.h"
#include "ulid.h"
#include "assert.h"

message_t* new_message(enum message_type type, union message_data data) {
    struct ulid_generator* generator = get_generator();
    message_t* res = (message_t*)malloc(sizeof(message_t));

    res->type = type;
    res->data = data;
    res->next = NULL;
    res->sender = pthread_self();
    char encoded[27];
    ulid_generate(generator, encoded);
    ulid_decode(res->ulid, encoded);

    return res;
}

void print_message(message_t* message) {
    switch (message->type) {
        case BOAT_EMPTY:
            printf("Message { type = BOAT_EMPTY, data =\n  ");
            print_boat(&message->data.boat, false);
            printf("}\n");
            break;
        case BOAT_FULL:
            printf("Message { type = BOAT_FULL, data =\n  ");
            print_boat(&message->data.boat, false);
            printf("}\n");
            break;

        case TRUCK_EMPTY:
            printf("Message { type = TRUCK_EMPTY, data =\n  ");
            print_truck(message->data.truck, false);
            printf("}\n");
            break;
        case TRUCK_FULL:
            printf("Message { type = TRUCK_FULL, data =\n  ");
            print_truck(message->data.truck, false);
            printf("}\n");
            break;
        case TRUCK_NEW:
            printf("Message { type = TRUCK_NEW, data =\n  ");
            print_truck(message->data.truck, false);
            printf("}\n");
            break;

        case WAGON_EMPTY:
            printf("Message { type = WAGON_EMPTY, data =\n  ");
            print_wagon(message->data.wagon, false);
            printf("}\n");
            break;
        case WAGON_FULL:
            printf("Message { type = WAGON_FULL, data =\n  ");
            print_wagon(message->data.wagon, false);
            printf("}\n");
            break;
    }
}

void free_message(message_t* message) {
    message_t* current = message;
    while (current != NULL) {
        message_t* msg = current;
        current = msg->next;
        free(message);
    }
}
