#include "control_tower.h"
#include "assert.h"

control_tower_t new_control_tower() {
    control_tower_t res;
    res.message_queue = NULL;

    pthread_mutexattr_t attributes;
    passert_eq(int, "%d", pthread_mutexattr_init(&attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_setpshared(&attributes, 1), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.message_mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_destroy(&attributes), 0);

    pthread_condattr_t cond_attributes;
    passert_eq(int, "%d", pthread_condattr_init(&cond_attributes), 0);
    passert_eq(int, "%d", pthread_condattr_setpshared(&cond_attributes, 1), 0);
    passert_eq(int, "%d", pthread_cond_init(&res.message_monitor, &cond_attributes), 0);
    passert_eq(int, "%d", pthread_condattr_destroy(&cond_attributes), 0);

    return res;
}

void free_control_tower(control_tower_t* control_tower) {
    free_message(control_tower->message_queue);

    pthread_mutex_destroy(&control_tower->message_mutex);
    pthread_cond_destroy(&control_tower->message_monitor);
}

void control_tower_send(control_tower_t* tower, message_t* message) {
    // S(γ).P()
    passert_eq(int, "%d", pthread_mutex_lock(&tower->message_mutex), 0);

    // Q(γ).send(message)
    message_t* current_message = tower->message_queue;
    if (current_message == NULL) {
        tower->message_queue = message;
    } else {
        while (current_message->next != NULL) {
            current_message = current_message->next;
        }
        current_message->next = message;
    }

    // M(γ).signal(S(γ))
    passert_eq(int, "%d", pthread_cond_broadcast(&tower->message_monitor), 0);
    passert_eq(int, "%d", pthread_mutex_unlock(&tower->message_mutex), 0);
}

message_t* control_tower_receive(control_tower_t* tower) {
    // M(γ).wait()
    passert_eq(int, "%d", pthread_mutex_lock(&tower->message_mutex), 0);

    if (tower->message_queue != NULL) {
        // message = Q(γ).read()
        message_t* res = tower->message_queue;
        tower->message_queue = res->next;
        res->next = NULL;
        passert_eq(int, "%d", pthread_mutex_unlock(&tower->message_mutex), 0);
        return res;
    }
    passert_eq(int, "%d", pthread_cond_wait(&tower->message_monitor, &tower->message_mutex), 0);

    // message = Q(γ).read()
    message_t* res = tower->message_queue;
    passert_neq(message_t*, "%p", res, NULL);

    tower->message_queue = res->next;
    res->next = NULL;

    // S(γ).V()
    passert_eq(int, "%d", pthread_mutex_unlock(&tower->message_mutex), 0);

    return res;
}
