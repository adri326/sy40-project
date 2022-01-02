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

void control_tower_new_truck(control_tower_t* tower, truck_t* truck) {
    if (rand() % 2 == 0) {
        *truck = empty_truck(rand() % N_DESTINATIONS);
    } else {
        *truck = new_truck(rand() % N_DESTINATIONS);
    }

    union message_data msg_data;
    msg_data.truck = truck;

    message_t* message = new_message(TRUCK_NEW, msg_data);

    if (rand() % 2 == 0) {
        crane_send(tower->crane_alpha, message);
    } else {
        crane_send(tower->crane_beta, message);
    }
}

void control_tower_new_boat(control_tower_t* tower) {
    boat_t boat = new_boat(rand() % N_DESTINATIONS, rand() % (BOAT_CONTAINERS - 1) + 1);

    boat_lane_t* boat_lane = &tower->crane_alpha->boat_lane;

    boat_lane_lock(boat_lane);
    boat_deque_push_back(boat_lane->queue, boat);
    boat_lane_unlock(boat_lane);
}

#define N_TRUCKS 10
#define N_BOATS 10

void* control_tower_entry(void* data) {
    control_tower_t* control_tower = (control_tower_t*)data;

    // Create a bunch of trucks :)
    truck_t* trucks = malloc(sizeof(truck_t) * N_TRUCKS);
    for (size_t n = 0; n < N_TRUCKS; n++) {
        control_tower_new_truck(control_tower, &trucks[n]);
    }

    for (size_t n = 0; n < N_BOATS; n++) {
        control_tower_new_boat(control_tower);
    }

    while (true) {
        message_t* message = control_tower_receive(control_tower);

        print_message(message);

        switch (message->type) {
            case TRUCK_NEW:
                passert(false, "Control tower may not receive a TRUCK_NEW message!\n");
                break;
            case TRUCK_FULL: { // truck is full, send it away and generate a new one
                truck_t* truck = message->data.truck;
                printf("Truck => %s (%zu)\n", DESTINATION_NAMES[truck->destination], truck->destination);

                control_tower_new_truck(control_tower, truck);
                break;
            }
            case TRUCK_EMPTY: { // truck is empty, move it to the other crane
                truck_t* truck = message->data.truck;
                truck->loading = true;

                union message_data msg_data;
                msg_data.truck = truck;

                message_t* message = new_message(TRUCK_EMPTY, msg_data);

                if (message->sender == control_tower->crane_alpha->thread) {
                    crane_send(control_tower->crane_beta, message);
                } else {
                    crane_send(control_tower->crane_alpha, message);
                }
                break;
            }
            case BOAT_FULL: { // boat is full, send it away and generate a new one
                boat_t boat = message->data.boat;
                printf("Boat => %s (%zu)\n", DESTINATION_NAMES[boat.destination], boat.destination);

                control_tower_new_boat(control_tower);
                break;
            }
            case BOAT_EMPTY: { // boat is empty, move it to crane_beta
                boat_t boat = message->data.boat;
                print_boat(&boat, true);

                boat_lane_t* boat_lane = &control_tower->crane_beta->boat_lane;

                boat_lane_lock(boat_lane);
                boat_deque_push_back(boat_lane->queue, boat);
                boat_lane_unlock(boat_lane);
                break;
            }
        }

        // crane_send(&crane_beta, message);

        free_message(message);
    }

    // print_boat(&boat, true);
    pthread_exit(NULL);
}
