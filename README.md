# sandbox

A minimal Linux container runtime built from raw syscalls in C++ — no
`libcontainer`, no `runc` internals reused, no high-level sandboxing
libraries. This project implements process and filesystem isolation
primitives directly (`clone`, `unshare`, `mount`, `pivot_root`) to understand
how container runtimes actually work under the hood.

## What it does

`sandbox` runs a command inside an isolated environment with:

- **Process isolation** — the sandboxed process gets its own PID namespace
  (it sees itself as PID 1), UTS namespace (independent hostname), and IPC
  namespace, so it can't see or signal processes on the host.
- **Filesystem isolation** — the sandboxed process is confined to a
  dedicated root filesystem via `pivot_root` (not plain `chroot`), with a
  private mount namespace so mount/unmount events don't propagate to the
  host and vice versa. `/proc`, `/sys`, and `/dev` are freshly mounted inside
  the new root so tools like `ps` behave correctly.

## Planned / in progress

- Resource limits via cgroups v2 (memory, CPU, PID count)
- Network namespace isolation with a veth pair for outbound connectivity
- Capability dropping and seccomp-bpf syscall filtering
- Rootless mode via user namespaces
- Config-file driven setup (rootfs path, limits, capabilities, command)

## Build

Requires a Linux system with kernel support for namespaces and cgroups v2
(most modern distros). Tested inside a disposable Ubuntu VM.

```bash
sudo apt install -y build-essential cmake libcap-dev libseccomp-dev
git clone https://github.com/<your-username>/sandbox.git
cd sandbox
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
sudo ./sandbox run <rootfs-path> <command> [args...]
```

Example:
```bash
sudo ./sandbox run ./test-rootfs /bin/sh
```

You'll need a real root filesystem to point at — see [Building a test
rootfs](#building-a-test-rootfs) below.

Root privileges are required for the namespace and mount syscalls used
internally (`CLONE_NEWNS`, `CLONE_NEWPID`, `pivot_root`, etc.).

## Building a test rootfs

A minimal Debian rootfs via `debootstrap`:
```bash
sudo debootstrap --variant=minbase focal ./test-rootfs http://archive.ubuntu.com/ubuntu/
```

Or export an Alpine image from Docker if you have it available:
```bash
docker export $(docker create alpine) | sudo tar -C ./test-rootfs -xf -
```

## How it works (brief)

1. `clone()` is called with `CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWIPC |
   CLONE_NEWNS` to create the child process directly inside a new set of
   namespaces.
2. Inside the child, the mount namespace is first made private
   (`MS_PRIVATE | MS_REC` on `/`) to prevent mount event propagation to/from
   the host.
3. `pivot_root()` swaps the process's root filesystem to the target rootfs
   directory, and the old root is unmounted.
4. `/proc`, `/sys`, and a minimal `/dev` are mounted inside the new root so
   standard tools function correctly in the isolated environment.
5. The target command is executed via `execve()` inside the fully isolated
   environment.

## ⚠️ Safety note

This project performs low-level mount and namespace operations that can
affect the host system if run incorrectly. **Do not run this on a machine
you care about.** Development and testing should be done inside a disposable
VM that you can snapshot and revert.

## References

- `man 2 clone`, `man 7 namespaces`, `man 2 pivot_root`, `man 7 mount_namespaces`
- Michael Kerrisk, *The Linux Programming Interface*
- [OCI Runtime Specification](https://github.com/opencontainers/runtime-spec)

## License

MIT 
