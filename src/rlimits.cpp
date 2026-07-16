#include "rlimits.hpp"

#include <cstdio>
#include <sys/resource.h>

void set_resource_limits() {
    struct rlimit rl;

    printf("[sandbox] applying resource limits...\n");

    rl.rlim_cur = 256; rl.rlim_max = 512;
    if (setrlimit(RLIMIT_NOFILE, &rl) == -1)
        perror("setrlimit RLIMIT_NOFILE");
    else
        printf("[sandbox] RLIMIT_NOFILE set: soft=%llu hard=%llu\n",
               (unsigned long long)rl.rlim_cur, (unsigned long long)rl.rlim_max);

    rl.rlim_cur = rl.rlim_max = 64; // see UID caveat: counted per real UID, not per cgroup
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
