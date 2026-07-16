#pragma once

// Creates the cgroup directory and applies pid/memory/cpu limits.
// Returns true on success. cgroup_path_out receives the full path,
// e.g. "/sys/fs/cgroup/sandbox_1234"
bool cgroup_create(const char* name, char* cgroup_path_out, size_t path_len);

// Adds the calling process (getpid()) to the given cgroup.
// Must be called from INSIDE the process that should be constrained,
// before execve().
bool cgroup_add_self(const char* cgroup_path);

// Removes the cgroup directory. Must be called after the member
// process has already exited — a cgroup can't be rmdir'd while it
// still has processes in cgroup.procs.
void cgroup_cleanup(const char* cgroup_path);
