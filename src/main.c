#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "assert.h"
#include "container.h"

void wait_success(pid_t pid, char* name);

int main(int argc, char* argv[]) {
    pid_t crane_alpha = fork();
    passert_gte(pid_t, "%d", crane_alpha, 0, "fork() should return a positive PID");

    if (crane_alpha == 0) {
        container_t container = new_container(0);
        print_container(&container, true);
        exit(0);
    }

    pid_t crane_beta = fork();
    passert_gte(pid_t, "%d", crane_beta, 0, "fork() should return a positive PID");

    if (crane_beta == 0) {
        container_t container = new_container(1);
        print_container(&container, true);
        exit(0);
    }

    pid_t control_tower = fork();
    passert_gte(pid_t, "%d", control_tower, 0, "fork() should return a positive PID");

    if (control_tower == 0) {
        exit(0);
    }

    wait_success(crane_alpha, "crane_alpha");
    wait_success(crane_beta, "crane_beta");
    wait_success(control_tower, "control_tower");
}

void wait_success(pid_t pid, char* name) {
    int status;
    waitpid(pid, &status, 0);
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
