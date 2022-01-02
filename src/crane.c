#include "crane.h"
#include "assert.h"

crane_t new_crane() {
    crane_t res;
    res.message_queue = NULL;

    pthread_mutexattr_t attributes;
    passert_eq(int, "%d", pthread_mutexattr_init(&attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_setpshared(&attributes, 1), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.message_mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_destroy(&attributes), 0);

    res.boat_lane = new_boat_lane();
    res.train_lane = new_train_lane();

    return res;
}

void free_crane(crane_t* crane) {
    free_message(crane->message_queue);
    free_boat_lane(&crane->boat_lane);
    free_train_lane(&crane->train_lane);

    pthread_mutex_destroy(&crane->message_mutex);
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
