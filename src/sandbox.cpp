#include "sandbox.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstring>
#include <sys/syscall.h>
#include <sys/resource.h>


static const int STACK_SIZE = 1024 * 1024;
static char child_stack[STACK_SIZE];

static char* ROOTFS = nullptr;
static char* const* CHILD_ARGV = nullptr;


static void set_resource_limits() {
    struct rlimit rl;

    printf("[sandbox] applying resource limits...\n");

    rl.rlim_cur = 256; rl.rlim_max = 512;
    if (setrlimit(RLIMIT_NOFILE, &rl) == -1)
        perror("setrlimit RLIMIT_NOFILE");
    else
        printf("[sandbox] RLIMIT_NOFILE set: soft=%llu hard=%llu\n",
               (unsigned long long)rl.rlim_cur, (unsigned long long)rl.rlim_max);

    rl.rlim_cur = rl.rlim_max = 64; // see UID caveat above
    if (setrlimit(RLIMIT_NPROC, &rl) == -1)
        perror("setrlimit RLIMIT_NPROC");
    else
        printf("[sandbox] RLIMIT_NPROC set: soft=%llu hard=%llu\n",
               (unsigned long long)rl.rlim_cur, (unsigned long long)rl.rlim_max);

    rl.rlim_cur = rl.rlim_max = 256UL * 1024 * 1024; // 256MB
    if (setrlimit(RLIMIT_AS, &rl) == -1)
        perror("setrlimit RLIMIT_AS");
    else
        printf("[sandbox] RLIMIT_AS set: %llu MB\n",
               (unsigned long long)(rl.rlim_cur / (1024 * 1024)));

    rl.rlim_cur = 30; rl.rlim_max = 60; // seconds
    if (setrlimit(RLIMIT_CPU, &rl) == -1)
        perror("setrlimit RLIMIT_CPU");
    else
        printf("[sandbox] RLIMIT_CPU set: soft=%llus hard=%llus\n",
               (unsigned long long)rl.rlim_cur, (unsigned long long)rl.rlim_max);

    rl.rlim_cur = rl.rlim_max = 100UL * 1024 * 1024; // 100MB
    if (setrlimit(RLIMIT_FSIZE, &rl) == -1)
        perror("setrlimit RLIMIT_FSIZE");
    else
        printf("[sandbox] RLIMIT_FSIZE set: %llu MB\n",
               (unsigned long long)(rl.rlim_cur / (1024 * 1024)));

    rl.rlim_cur = rl.rlim_max = 0;

    if (setrlimit(RLIMIT_CORE, &rl) == -1)
        perror("setrlimit RLIMIT_CORE");
    else
        printf("[sandbox] RLIMIT_CORE disabled\n");

    if (setrlimit(RLIMIT_MEMLOCK, &rl) == -1)
        perror("setrlimit RLIMIT_MEMLOCK");
    else
        printf("[sandbox] RLIMIT_MEMLOCK disabled\n");

    if (setrlimit(RLIMIT_NICE, &rl) == -1)
        perror("setrlimit RLIMIT_NICE");
    else
        printf("[sandbox] RLIMIT_NICE disabled\n");

    if (setrlimit(RLIMIT_RTPRIO, &rl) == -1)
        perror("setrlimit RLIMIT_RTPRIO");
    else
        printf("[sandbox] RLIMIT_RTPRIO disabled\n");

    printf("[sandbox] resource limits applied\n");
}


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
    //set the old rootfs somewhere unitl its unmounted
    char old_root[256];
    snprintf(old_root, sizeof(old_root), "%s/oldroot", ROOTFS);
    mkdir(old_root, 0755);

    // pivot_root is not a  glbc wrapper function that can reliably call directly in all environments.
    if (syscall(SYS_pivot_root, ROOTFS, old_root) == -1) {
        perror("pivot_root");
        return 1;
    }

    chdir("/");
    umount2("/oldroot", MNT_DETACH);

    sethostname("sandbox", 7);
    set_resource_limits();
    printf("%s\n", CHILD_ARGV[0]);
    execvp(CHILD_ARGV[0], CHILD_ARGV);

    perror("execvp");
    return 1;
}

void run_sandbox(const char* rootfs, char* const argv[]) {
    ROOTFS = const_cast<char*>(rootfs);
    CHILD_ARGV = argv;

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
        exit(1);
    }

    waitpid(pid, nullptr, 0);
}

