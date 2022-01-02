#include "crane.h"
#include "assert.h"
#include <unistd.h>

crane_t new_crane(bool load_boats, bool load_trains) {
    crane_t res;
    res.message_queue = NULL;

    res.load_boats = load_boats;
    res.load_trains = load_trains;

    pthread_mutexattr_t attributes;
    passert_eq(int, "%d", pthread_mutexattr_init(&attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_setpshared(&attributes, 1), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.message_mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.stuck_mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_destroy(&attributes), 0);

    res.boat_lane = new_boat_lane();
    res.train_lane = new_train_lane();
    res.truck_lane = new_truck_lane();

    res.stuck = false;
    res.boats_cycled = 0;

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

void crane_notify_boat(crane_t* crane, enum message_type type) {
    union message_data msg_data;
    msg_data.boat = crane->boat_lane.current_boat;
    crane->boat_lane.has_current_boat = false;

    control_tower_send(crane->control_tower, new_message(type, msg_data));
}

void crane_notify_truck(crane_t* crane, enum message_type type, truck_t* truck) {
    union message_data msg_data;
    msg_data.truck = truck;

    passert(truck_lane_remove(&crane->truck_lane, truck), "Truck isn't in the truck lane!\n");

    control_tower_send(crane->control_tower, new_message(type, msg_data));
}

void crane_notify_wagon(crane_t* crane, enum message_type type, wagon_t* wagon) {
    union message_data msg_data;
    msg_data.wagon = wagon;

    control_tower_send(crane->control_tower, new_message(type, msg_data));
}

bool crane_unload(crane_t* crane, container_holder_t* holder) {
    size_t destination = holder->container.destination;

    if (crane->load_boats && crane->boat_lane.has_current_boat) { // Try to unload a container onto the current boat
        boat_t* boat = &crane->boat_lane.current_boat;

        if (boat->destination == destination && !boat_is_full(boat)) {
            transfer_container(
                holder,
                boat_first_empty(boat)
            );
            if (boat_is_full(boat)) {
                crane_notify_boat(crane, BOAT_FULL);
            }
            return true;
        }
    }

    if (crane->load_trains) { // Try to unload a container onto a train
        wagon_t* wagon;
        train_lane_lock(&crane->train_lane);
        if ((wagon = train_lane_accepts(&crane->train_lane, destination))) {
            transfer_container(
                holder,
                wagon_first_empty(wagon)
            );
            train_lane_unlock(&crane->train_lane);

            if (wagon_is_full(wagon)) {
                crane_notify_wagon(crane, WAGON_FULL, wagon);
            }
            return true;
        }
        train_lane_unlock(&crane->train_lane);
    }

    // Otherwise, try to unload the container onto a truck
    truck_t* truck = truck_lane_accepts(&crane->truck_lane, destination);
    if (truck != NULL) {
        transfer_container(
            holder,
            &truck->container
        );

        crane_notify_truck(crane, TRUCK_FULL, truck);
        return true;
    } else {
        return false;
    }
}

void crane_handle_message(crane_t* crane, message_t* message) {
    switch (message->type) {
        case TRUCK_NEW:
        case TRUCK_EMPTY:
            truck_lane_push(&crane->truck_lane, message->data.truck);
            break;
        case CRANE_STUCK:
            pthread_exit(NULL);
            break;
        default:
            // noop
            break;
    }
}

void* crane_entry(void* data) {
    crane_t* crane = (crane_t*)data;

    usleep(100000);
    while (true) {
        message_t* msg = crane_receive(crane);

        if (msg != NULL) {
            crane_handle_message(crane, msg);
            free_message(msg);
        } else {
            break;
        }
    }

    print_crane(crane);

    bool could_move = true;
    while (true) {
        message_t* msg = crane_receive(crane);

        if (msg != NULL) {
            crane_handle_message(crane, msg);
            free_message(msg);
        } else if (!could_move) {
            // print_crane(crane);
        }

        // Let a boat in
        if (!crane->boat_lane.has_current_boat) {
            boat_lane_lock(&crane->boat_lane);
            if (boat_deque_pop_front(crane->boat_lane.queue, &crane->boat_lane.current_boat)) {
                crane->boat_lane.has_current_boat = true;
                // printf("A boat stops at the crane!\n");
            }
            boat_lane_unlock(&crane->boat_lane);
        }

        // Try to move a container
        could_move = false;

        // Unload from the boat lane
        if (!crane->load_boats && crane->boat_lane.has_current_boat) {
            boat_t* boat = &crane->boat_lane.current_boat;
            bool has_cargo = false;
            for (size_t n = 0; n < BOAT_CONTAINERS; n++) {
                if (boat->containers[n].is_empty) continue;

                if (crane_unload(crane, &boat->containers[n])) {
                    could_move = true;
                    // printf("SUCCESS!\n");
                } else {
                    has_cargo = true;
                }
            }

            if (!has_cargo) {
                crane_notify_boat(crane, BOAT_EMPTY);
            }
        }

        // Unload from the train lane
        if (!crane->load_trains) {
            train_lane_lock(&crane->train_lane);

            for (size_t n = 0; n < crane->train_lane.n_wagons; n++) {
                wagon_t* wagon = crane->train_lane.wagons[n];
                if (wagon_is_empty(wagon)) continue;

                for (size_t o = 0; o < WAGON_CONTAINERS; o++) {
                    if (wagon->containers[o].is_empty) continue;

                    if (crane_unload(crane, &wagon->containers[o])) {
                        could_move = true;
                        // printf("SUCCESS!\n");
                    }
                }

                if (wagon_is_empty(wagon)) {
                    crane_notify_wagon(crane, WAGON_EMPTY, wagon);
                }
            }

            train_lane_unlock(&crane->train_lane);
        }

        // Unload from the truck lane
        struct truck_ll* current_truck = crane->truck_lane.trucks;
        while (current_truck != NULL) {
            truck_t* truck = current_truck->truck;
            if (!truck->loading) {
                if (crane_unload(crane, &truck->container)) {
                    could_move = true;
                    // printf("SUCCESS!\n");
                    current_truck = current_truck->next;
                    crane_notify_truck(crane, TRUCK_EMPTY, truck);
                    continue;
                }
            }
            current_truck = current_truck->next;
        }

        if (!could_move && crane->boat_lane.has_current_boat) {
            boat_lane_lock(&crane->boat_lane);
            boat_deque_push_back(crane->boat_lane.queue, crane->boat_lane.current_boat);
            crane->boat_lane.has_current_boat = false;
            boat_lane_unlock(&crane->boat_lane);
            crane->boats_cycled++;
        }

        if (!could_move) {
            boat_lane_lock(&crane->boat_lane);
            if (crane->boats_cycled >= crane->boat_lane.queue->length) {
                boat_lane_unlock(&crane->boat_lane);
                // We are stuck

                pthread_mutex_lock(&crane->stuck_mutex);
                crane->stuck = true;
                pthread_mutex_unlock(&crane->stuck_mutex);
                union message_data msg_data;
                msg_data.stuck = true;

                control_tower_send(crane->control_tower, new_message(CRANE_STUCK, msg_data));
            } else {
                boat_lane_unlock(&crane->boat_lane);
            }
        }

        if (could_move) {
            crane->boats_cycled = 0;
        }
    }

    pthread_exit(NULL);
    return NULL;
}
