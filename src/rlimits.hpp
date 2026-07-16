#pragma once

// Applies the sandbox's per-process resource limits (RLIMIT_*).
// Must be called from inside the sandboxed process, before execve().
void set_resource_limits();
