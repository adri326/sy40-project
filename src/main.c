#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "assert.h"

int main(int argc, char* argv[]) {
    pid_t crane_alpha = fork();
    passert_gte(pid_t, "%d", crane_alpha, 0, "fork() should return a positive PID");

    if (crane_alpha == 0) {
        exit(0);
    }

    pid_t crane_beta = fork();
    passert_gte(pid_t, "%d", crane_beta, 0, "fork() should return a positive PID");

    if (crane_beta == 0) {
        exit(0);
    }



    int status;
    waitpid(crane_alpha, &status, 0);
    if (WIFSIGNALED(status)) {
        passert(
            WIFEXITED(status),
            "crane_alpha didn't terminate normally.\n" FMT_INFO("INFO") ": %s",
            strsignal(WTERMSIG(status))
        );
    } else {
        passert(WIFEXITED(status), "crane_alpha didn't terminate normally.");
    }

    passert_eq(int, "%d", WEXITSTATUS(status), 0, "crane_alpha didn't terminate with exit code 0.");

    waitpid(crane_beta, &status, 0);
    if (WIFSIGNALED(status)) {
        passert(
            WIFEXITED(status),
            "crane_beta didn't terminate normally.\n" FMT_INFO("INFO") ": %s",
            strsignal(WTERMSIG(status))
        );
    } else {
        passert(WIFEXITED(status), "crane_beta didn't terminate normally.");
    }
    passert_eq(int, "%d", WEXITSTATUS(status), 0, "crane_beta didn't terminate with exit code 0.");
}
