#define _GNU_SOURCE

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "sandbox.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <rootfs> <command> [args...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0) {
        run_sandbox(argv[1], &argv[2]);
        _exit(0);
    }

    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("child exited with %d\n", WEXITSTATUS(status));
        return 0;
    }

    perror("fork");
    return 1;
}
