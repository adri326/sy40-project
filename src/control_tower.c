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
    // if (control_tower->message_queue != NULL) {
    //     free_message(control_tower->message_queue);
    // }

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

void control_tower_transfer_wagons(control_tower_t* tower, train_t* train) {
    train_lane_t* lane_alpha = &tower->crane_alpha->train_lane;
    train_lane_t* lane_beta = &tower->crane_beta->train_lane;

    size_t n_wagons = 0;
    for (size_t n = train->offset; n < train->n_wagons; n++) {
        if (train->wagon_empty[n]) n_wagons++;
        else break;
    }
    train_lane_lock(lane_beta);
    train_lane_shift(lane_beta, n_wagons);
    train_lane_lock(lane_alpha);
    for (size_t n = train->offset; n < train->offset + n_wagons; n++) {
        train_lane_append(lane_alpha, &train->wagons[n]);
    }
    train->offset += n_wagons;
    train_lane_unlock(lane_beta);
    train_lane_unlock(lane_alpha);
}

void control_tower_new_train(control_tower_t* tower, train_t** train) {
    *train = new_train(rand() % N_DESTINATIONS, rand() % TRAIN_WAGONS);
    train_lane_lock(&tower->crane_beta->train_lane);
    for (size_t n = 0; n < (*train)->n_wagons; n++) {
        train_lane_append(&tower->crane_beta->train_lane, &(*train)->wagons[n]);
    }
    train_lane_unlock(&tower->crane_beta->train_lane);
}

void control_tower_send_train(control_tower_t* tower, train_t** train) {
    train_lane_t* lane_alpha = &tower->crane_alpha->train_lane;

    train_lane_lock(lane_alpha);
    train_lane_shift(lane_alpha, (*train)->n_wagons);
    train_lane_unlock(lane_alpha);

    control_tower_new_train(tower, train);
}

void* control_tower_entry(void* data) {
    srand(time(0));
    control_tower_t* control_tower = (control_tower_t*)data;

    // Create a bunch of trucks :)
    truck_t* trucks = malloc(sizeof(truck_t) * N_TRUCKS);
    for (size_t n = 0; n < N_TRUCKS; n++) {
        control_tower_new_truck(control_tower, &trucks[n]);
    }

    for (size_t n = 0; n < N_BOATS; n++) {
        control_tower_new_boat(control_tower);
    }

    train_t* trains[2];
    size_t first_train = 0;

    control_tower_new_train(control_tower, &trains[0]);
    control_tower_new_train(control_tower, &trains[1]);

    train_lane_print(&control_tower->crane_beta->train_lane, true);

    bool loop = true;
    while (loop) {
        message_t* message = control_tower_receive(control_tower);

        // print_message(message);

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
                // print_boat(&boat, true);

                boat_lane_t* boat_lane = &control_tower->crane_beta->boat_lane;

                boat_lane_lock(boat_lane);
                boat_deque_push_back(boat_lane->queue, boat);
                boat_lane_unlock(boat_lane);
                break;
            }
            case WAGON_EMPTY: { // wagon is empty, flag it as such
                // printf("WAGON_EMPTY\n");
                wagon_t* wagon = message->data.wagon;
                // print_wagon(wagon, true);

                for (size_t t = 0; t < 2; t++) {
                    for (size_t n = 0; n < trains[t]->n_wagons; n++) {
                        if (&trains[t]->wagons[n] == wagon) {
                            trains[t]->wagon_empty[n] = true;
                        }
                    }
                }

                // If the head wagons are empty
                // ISSUE: This may rarely fail
                if (
                    trains[first_train]->offset < trains[first_train]->n_wagons
                    && trains[first_train]->wagon_empty[trains[first_train]->offset]
                ) {
                    printf("Train %zu ... transfer\n", first_train);
                    control_tower_transfer_wagons(control_tower, trains[first_train]);
                    // train_lane_print(&control_tower->crane_alpha->train_lane, true);
                    // train_lane_print(&control_tower->crane_beta->train_lane, true);

                    if (trains[first_train]->offset == trains[first_train]->n_wagons) {
                        first_train = 1 - first_train;
                    }
                }
                break;
            }
            case WAGON_FULL: {
                // printf("WAGON_FULL\n");
                wagon_t* wagon = message->data.wagon;
                // print_wagon(wagon, true);

                for (size_t t = 0; t < 2; t++) {
                    bool is_full = true;
                    for (size_t n = 0; n < trains[t]->n_wagons; n++) {
                        if (&trains[t]->wagons[n] == wagon) {
                            trains[t]->wagon_full[n] = true;
                        }
                        if (!trains[t]->wagon_full[n]) is_full = false;
                    }
                    if (is_full) {
                        printf("Train => %s (%zu)\n", DESTINATION_NAMES[trains[t]->destination], trains[t]->destination);
                        control_tower_send_train(control_tower, &trains[t]);
                    }
                }
                break;
            }
            case CRANE_STUCK: {
                // Check if both cranes are stuck
                pthread_mutex_lock(&control_tower->crane_alpha->stuck_mutex);
                bool alpha_stuck = control_tower->crane_alpha->stuck;
                pthread_mutex_unlock(&control_tower->crane_alpha->stuck_mutex);
                pthread_mutex_lock(&control_tower->crane_alpha->stuck_mutex);
                bool beta_stuck = control_tower->crane_beta->stuck;
                pthread_mutex_unlock(&control_tower->crane_alpha->stuck_mutex);

                if (alpha_stuck && beta_stuck) {
                    union message_data msg_data;
                    msg_data.stuck = true;
                    message_t* message = new_message(CRANE_STUCK, msg_data);
                    crane_send(control_tower->crane_beta, message);
                    crane_send(control_tower->crane_alpha, message);
                    loop = false;
                }
            }
        }

        // crane_send(&crane_beta, message);

        free_message(message);
    }

    // print_boat(&boat, true);
    pthread_exit(NULL);
}
