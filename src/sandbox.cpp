
#include "sandbox.hpp"
#include "rlimits.hpp"
#include "cgroups.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstring>
#include <sys/syscall.h>

static const int STACK_SIZE = 1024 * 1024;
static char child_stack[STACK_SIZE];

static char* ROOTFS = nullptr;
static char* const* CHILD_ARGV = nullptr;
static char CGROUP_PATH[256] = {0};

static int child_func(void* arg) {
    (void)arg;

    printf("[sandbox] setting up namespace...\n");

    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
        perror("mount MS_PRIVATE");
        return 1;
    }

    if (mount(ROOTFS, ROOTFS, NULL, MS_BIND | MS_REC, NULL) == -1) {
        perror("mount bind rootfs");
        return 1;
    }

    // stash the old rootfs somewhere until it's unmounted
    char old_root[256];
    snprintf(old_root, sizeof(old_root), "%s/oldroot", ROOTFS);
    mkdir(old_root, 0755);

    // pivot_root is not a glibc wrapper function that can be reliably
    // called directly in all environments.
    if (syscall(SYS_pivot_root, ROOTFS, old_root) == -1) {
        perror("pivot_root");
        return 1;
    }

    chdir("/");
    umount2("/oldroot", MNT_DETACH);

    sethostname("sandbox", 7);

    set_resource_limits();

    if (!cgroup_add_self(CGROUP_PATH)) {
        fprintf(stderr, "[sandbox] warning: failed to join cgroup\n");
    }

    printf("%s\n", CHILD_ARGV[0]);
    execvp(CHILD_ARGV[0], CHILD_ARGV);

    perror("execvp");
    return 1;
}

void run_sandbox(const char* rootfs, char* const argv[]) {
    ROOTFS = const_cast<char*>(rootfs);
    CHILD_ARGV = argv;

    char cgroup_name[64];
    snprintf(cgroup_name, sizeof(cgroup_name), "sandbox_%d", getpid());
    cgroup_create(cgroup_name, CGROUP_PATH, sizeof(CGROUP_PATH));

    int flags =
        CLONE_NEWPID |
        CLONE_NEWUTS |
        CLONE_NEWIPC |
        CLONE_NEWNS |
        SIGCHLD;

    printf("[sandbox] cloning process...\n");

    pid_t pid = clone(
        child_func,
        child_stack + STACK_SIZE,
        flags,
        nullptr
    );

    if (pid == -1) {
        perror("clone");
        cgroup_cleanup(CGROUP_PATH);
        exit(1);
    }

    waitpid(pid, nullptr, 0);
    cgroup_cleanup(CGROUP_PATH);
}
