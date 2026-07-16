#include "cgroups.hpp"

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static bool write_file(const char* path, const char* value) {
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror(path);
        return false;
    }
    ssize_t len = (ssize_t)strlen(value);
    bool ok = write(fd, value, len) == len;
    if (!ok) perror(path);
    close(fd);
    return ok;
}

bool cgroup_create(const char* name, char* cgroup_path_out, size_t path_len) {
    // Enable controllers on the root cgroup so subdirectories can use them.
    // Safe to call repeatedly; harmless if already enabled.
    write_file("/sys/fs/cgroup/cgroup.subtree_control", "+pids +memory +cpu");

    snprintf(cgroup_path_out, path_len, "/sys/fs/cgroup/%s", name);

    if (mkdir(cgroup_path_out, 0755) == -1) {
        perror("mkdir cgroup");
        return false;
    }

    char file[512];

    snprintf(file, sizeof(file), "%s/pids.max", cgroup_path_out);
    write_file(file, "64");

    snprintf(file, sizeof(file), "%s/memory.max", cgroup_path_out);
    write_file(file, "268435456"); // 256MB

    snprintf(file, sizeof(file), "%s/cpu.max", cgroup_path_out);
    write_file(file, "50000 100000"); // 50% of one CPU

    printf("[sandbox] cgroup created at %s (pids=64, mem=256MB, cpu=50%%)\n",
           cgroup_path_out);

    return true;
}

bool cgroup_add_self(const char* cgroup_path) {
    char file[512];
    snprintf(file, sizeof(file), "%s/cgroup.procs", cgroup_path);

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", getpid());

    if (!write_file(file, pid_str)) return false;

    printf("[sandbox] pid %s joined cgroup %s\n", pid_str, cgroup_path);
    return true;
}

void cgroup_cleanup(const char* cgroup_path) {
    if (rmdir(cgroup_path) == -1) {
        perror("rmdir cgroup");
    } else {
        printf("[sandbox] cgroup %s removed\n", cgroup_path);
    }
}
