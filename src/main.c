#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include "assert.h"
#include "container.h"
#include "boat.h"
#include "control_tower.h"
#include "crane.h"

void lfork(pthread_t* res, void* (*entry)(void*));
void wait_success(pthread_t* thread, char* name);

static control_tower_t control_tower_gamma;
static crane_t crane_alpha, crane_beta;

void* crane_alpha_entry(void* data) {
    boat_t boat_1 = new_boat(2, 3);
    boat_t boat_2 = new_boat(1, 2);

    boat_lane_lock(&crane_alpha.boat_lane);

    boat_deque_push_back(crane_alpha.boat_lane.queue, boat_1);
    boat_deque_push_back(crane_alpha.boat_lane.queue, boat_2);

    boat_deque_print(crane_alpha.boat_lane.queue, true);

    union message_data msg_data;
    passert(boat_deque_pop_front(crane_alpha.boat_lane.queue, &msg_data.boat));
    message_t* msg = new_message(BOAT_EMPTY, msg_data);
    control_tower_send(&control_tower_gamma, msg);

    boat_lane_unlock(&crane_alpha.boat_lane);

    pthread_exit(NULL);
}

void* crane_beta_entry(void* data) {
    container_t container = new_container(1);
    // print_container(&container, true);

    truck_t t = new_truck(3);
    truck_lane_push(&crane_beta.truck_lane, &t);
    truck_lane_print(&crane_beta.truck_lane, true);

    while (true) {
        message_t* message = crane_receive(&crane_beta);
        if (message != NULL) {
            print_message(message);
            boat_lane_lock(&crane_beta.boat_lane);
            boat_deque_push_back(crane_beta.boat_lane.queue, message->data.boat);
            boat_lane_unlock(&crane_beta.boat_lane);
            break;
        }
    }

    print_crane(&crane_beta);

    pthread_exit(NULL);
}

void* control_tower_entry(void* data) {
    while (true) {
        message_t* message = control_tower_receive(&control_tower_gamma);

        print_message(message);

        crane_send(&crane_beta, message);

        free_message(message);
        break;
    }

    // print_boat(&boat, true);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    control_tower_gamma = new_control_tower();
    crane_alpha = new_crane(false, true);
    crane_beta = new_crane(true, false);

    lfork(&crane_alpha.thread, crane_alpha_entry);
    lfork(&crane_beta.thread, crane_beta_entry);
    lfork(&control_tower_gamma.thread, control_tower_entry);

    wait_success(&crane_alpha.thread, "crane_alpha");
    wait_success(&crane_beta.thread, "crane_beta");
    wait_success(&control_tower_gamma.thread, "control_tower");

    free_control_tower(&control_tower_gamma);
    free_crane(&crane_alpha);
    free_crane(&crane_beta);
}

void lfork(pthread_t* res, void* (*entry)(void*)) {
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    passert_eq(int, "%d", pthread_create(res, &attributes, entry, NULL), 0);
}

void wait_success(pthread_t* thread, char* name) {
    passert_eq(
        int, "%d",
        pthread_join(*thread, NULL), 0,
        "Thread %s didn't terminate normally.", name
    );
}
