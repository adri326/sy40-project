#include "crane.h"
#include "assert.h"

crane_t new_crane(bool load_boats, bool load_trains) {
    crane_t res;
    res.message_queue = NULL;

    res.load_boats = load_boats;
    res.load_trains = load_trains;

    pthread_mutexattr_t attributes;
    passert_eq(int, "%d", pthread_mutexattr_init(&attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_setpshared(&attributes, 1), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.message_mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_destroy(&attributes), 0);

    res.boat_lane = new_boat_lane();
    res.train_lane = new_train_lane();
    res.truck_lane = new_truck_lane();

    return res;
}

void free_crane(crane_t* crane) {
    free_message(crane->message_queue);
    free_boat_lane(&crane->boat_lane);
    free_train_lane(&crane->train_lane);
    free_truck_lane(&crane->truck_lane);

    pthread_mutex_destroy(&crane->message_mutex);
}

void print_crane(crane_t* crane) {
    printf(
        "=== Crane { load_boats = %s, load_trains = %s } ===\n",
        crane->load_boats ? "true" : "false",
        crane->load_trains ? "true" : "false"
    );
    boat_lane_print(&crane->boat_lane, true);
    train_lane_print(&crane->train_lane, true);
    truck_lane_print(&crane->truck_lane, true);
    printf("=== ~ ===\n");
}

void crane_send(crane_t* crane, message_t* message) {
    // S(τ).P()
    passert_eq(int, "%d", pthread_mutex_lock(&crane->message_mutex), 0);

    // Q(τ).send(message)
    message_t* current_message = crane->message_queue;
    if (current_message == NULL) {
        crane->message_queue = message;
    } else {
        while (current_message->next != NULL) {
            current_message = current_message->next;
        }
        current_message->next = message;
    }

    // S(τ).V()
    passert_eq(int, "%d", pthread_mutex_unlock(&crane->message_mutex), 0);
}

message_t* crane_receive(crane_t* crane) {
    // S(τ).P()
    passert_eq(int, "%d", pthread_mutex_lock(&crane->message_mutex), 0);

    if (crane->message_queue != NULL) {
        // message = Q(τ).read()
        message_t* res = crane->message_queue;
        crane->message_queue = res->next;
        res->next = NULL;
        // S(τ).V()
        passert_eq(int, "%d", pthread_mutex_unlock(&crane->message_mutex), 0);
        return res;
    } else {
        passert_eq(int, "%d", pthread_mutex_unlock(&crane->message_mutex), 0);
        return NULL;
    }
}
