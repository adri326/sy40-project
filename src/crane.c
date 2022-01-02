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

bool crane_unload(crane_t* crane, container_holder_t* holder) {
    size_t destination = holder->container.destination;

    if (crane->load_boats && crane->boat_lane.has_current_boat) { // Try to unload a container onto the current boat
        boat_t* boat = &crane->boat_lane.current_boat;

        if (boat->destination == destination && !boat_is_full(boat)) {
            transfer_container(
                holder,
                boat_first_empty(boat)
            );
            // TODO: notify γ if the boat is full
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
            // TODO: notify γ if the wagon is full
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
        // TODO: notify γ if the truck is full
        return true;
    } else {
        return false;
    }
}

void* crane_entry(void* data) {
    crane_t* crane = (crane_t*)data;

    print_crane(crane);

    bool could_move = true;
    while (true) {
        message_t* msg = crane_receive(crane);

        if (msg != NULL) {
            // handle message
            free_message(msg);
        }

        // Let a boat in
        if (!crane->boat_lane.has_current_boat) {
            boat_lane_lock(&crane->boat_lane);
            if (boat_deque_pop_front(crane->boat_lane.queue, &crane->boat_lane.current_boat)) {
                crane->boat_lane.has_current_boat = true;
                printf("A boat stops at the crane!\n");
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
                    printf("SUCCESS!\n");
                    // TODO: notify γ if the boat is empty
                    break;
                } else {
                    has_cargo = true;
                }
            }

            if (!has_cargo) {
                boat_lane_lock(&crane->boat_lane);
                boat_lane_unlock(&crane->boat_lane);
            }

            if (could_move) continue;
        }
    }

    pthread_exit(NULL);
    return NULL;
}
