# NetAgent

Linux network monitoring and diagnostics project written in C.

## Goals

- Learn Linux systems programming
- Understand Linux networking internals
- Work with sockets, Netlink and raw packets
- Build and run on both x86_64 Debian VM and ARM64 Raspberry Pi
- Implement network measurement and diagnostics tools

## Build

```bash
cmake -S . -B build
cmake --build build
