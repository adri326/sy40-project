#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include "assert.h"
#include "container.h"
#include "boat.h"

#define USE_THREADS

#ifdef USE_THREADS
    #define THREAD_TYPE pthread_t
#else
    #define THREAD_TYPE pid_t
#endif

void lfork(THREAD_TYPE* res, void* (*entry)(void*));
void wait_success(THREAD_TYPE* thread, char* name);

static boat_lane_t boat_lane_alpha, boat_lane_beta;

void* crane_alpha_entry(void* data) {
    boat_t boat_1 = new_boat(2, 3);
    boat_t boat_2 = new_boat(1, 2);

    boat_lane_lock(&boat_lane_alpha);

    boat_deque_push_back(boat_lane_alpha.queue, boat_1);
    boat_deque_push_back(boat_lane_alpha.queue, boat_2);

    boat_deque_print(boat_lane_alpha.queue, true);

    boat_lane_unlock(&boat_lane_alpha);

    pthread_exit(NULL);
}

void* crane_beta_entry(void* data) {
    container_t container = new_container(1);
    // print_container(&container, true);
    pthread_exit(NULL);
}

void* control_tower_entry(void* data) {

    // print_boat(&boat, true);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    THREAD_TYPE crane_alpha, crane_beta, control_tower;

    boat_lane_alpha = new_boat_lane();
    boat_lane_beta = new_boat_lane();

    lfork(&crane_alpha, crane_alpha_entry);
    lfork(&crane_beta, crane_beta_entry);
    lfork(&control_tower, control_tower_entry);

    wait_success(&crane_alpha, "crane_alpha");
    wait_success(&crane_beta, "crane_beta");
    wait_success(&control_tower, "control_tower");

    free_boat_lane(&boat_lane_alpha);
    free_boat_lane(&boat_lane_beta);
}

#ifdef USE_THREADS
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
#else
    void lfork(pid_t* res, void* (*entry)(void*)) {
        *res = fork();
        passert_gte(pid_t, "%d", *res, 0, "fork() should return a positive PID");

        if (*res == 0) {
            entry(NULL);
            exit(0);
        }
    }

    void wait_success(pid_t* pid, char* name) {
        int status;
        waitpid(*pid, &status, 0);
        if (WIFSIGNALED(status)) {
            passert(
                WIFEXITED(status),
                "%s didn't terminate normally.\n" FMT_INFO("INFO") ": %s",
                name,
                strsignal(WTERMSIG(status))
            );
        } else {
            passert(WIFEXITED(status), "%s didn't terminate normally.", name);
        }
        passert_eq(int, "%d", WEXITSTATUS(status), 0, "%s didn't terminate with exit code 0.", name);
    }
#endif