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

void lfork(pthread_t* res, void* (*entry)(void*), void* data);
void wait_success(pthread_t* thread, char* name);

static control_tower_t control_tower_gamma;
static crane_t crane_alpha, crane_beta;


int main(int argc, char* argv[]) {
    control_tower_gamma = new_control_tower();
    crane_alpha = new_crane(false, true);
    crane_beta = new_crane(true, false);

    crane_alpha.control_tower = &control_tower_gamma;
    crane_beta.control_tower = &control_tower_gamma;
    control_tower_gamma.crane_alpha = &crane_alpha;
    control_tower_gamma.crane_beta = &crane_beta;

    boat_deque_push_back(crane_alpha.boat_lane.queue, new_boat(1, 5));
    truck_t truck = empty_truck(2);
    truck_lane_push(&crane_alpha.truck_lane, &truck);

    lfork(&crane_alpha.thread, crane_entry, (void*)&crane_alpha);
    lfork(&crane_beta.thread, crane_entry, (void*)&crane_beta);
    lfork(&control_tower_gamma.thread, control_tower_entry, (void*)&control_tower_gamma);

    wait_success(&crane_alpha.thread, "crane_alpha");
    wait_success(&crane_beta.thread, "crane_beta");
    wait_success(&control_tower_gamma.thread, "control_tower");

    free_control_tower(&control_tower_gamma);
    free_crane(&crane_alpha);
    free_crane(&crane_beta);
}

void lfork(pthread_t* res, void* (*entry)(void*), void* data) {
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    passert_eq(int, "%d", pthread_create(res, &attributes, entry, data), 0);
}

void wait_success(pthread_t* thread, char* name) {
    passert_eq(
        int, "%d",
        pthread_join(*thread, NULL), 0,
        "Thread %s didn't terminate normally.", name
    );
}
