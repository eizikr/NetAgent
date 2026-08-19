# NetAgent

NetAgent is a Linux networking learning project written in C. The goal
is to build a packet analyzer step by step, from raw Ethernet capture
through protocol parsing and, later, more advanced Linux
packet-processing mechanisms.

The project is designed to be developed on a Debian VM and later run on
a Raspberry Pi 4.

## Current Status

NetAgent currently supports:

-   Live Layer 2 packet capture with Linux `AF_PACKET` raw sockets.
-   Binding capture to a selected network interface.
-   IPv4-only capture at the packet socket using `ETH_P_IP`.
-   Clean shutdown with `Ctrl+C` / `SIGINT`.
-   Ethernet II parsing.
-   IPv4 parsing, including IHL-based header length.
-   IPv4 protocol dispatch.
-   ICMP Echo Request / Echo Reply parsing.
-   UDP parsing and payload boundary validation.
-   Userspace filtering of UDP traffic by a selected port.
-   Parsing the initial fixed fields of a TWAMP Session-Sender payload.
-   Processing both synthetic test vectors and real captured frames.

The next milestone is kernel-side filtering with classic BPF.

## Packet Flow

``` text
Network
   |
   v
NIC / Virtual NIC
   |
   v
Driver
   |
   v
Linux Kernel
   |
   v
AF_PACKET / SOCK_RAW / ETH_P_IP
   |
   v
recvfrom()
   |
   v
capture.c
   |
   v
process_packet()
   |
   v
Ethernet Parser
   |
   v
IPv4 Parser
   |
   +------ ICMP --> ICMP Parser
   |
   +------ TCP  --> planned
   |
   +------ UDP  --> UDP Parser --> TWAMP Parser
```

A central design decision is that parsers receive only a byte pointer
and an explicit length. They therefore do not care whether the bytes
originated from a hardcoded test array, a PCAP test vector, or a live
raw socket.

## Project Structure

``` text
netagent/
├── CMakeLists.txt
├── README.md
├── config/
├── docs/
├── include/
│   └── netagent/
│       ├── capture.h
│       ├── ethernet.h
│       ├── icmp.h
│       ├── ipv4.h
│       ├── packet.h
│       ├── twamp.h
│       └── udp.h
├── scripts/
├── src/
│   ├── capture.c
│   ├── ethernet.c
│   ├── icmp.c
│   ├── ipv4.c
│   ├── main.c
│   ├── packet.c
│   ├── twamp.c
│   └── udp.c
└── tests/
```

## Modules

### `main.c`

Program entry point. The capture interface is supplied on the command
line rather than being hardcoded.

``` bash
sudo ./build/cmake/netagent ens33
```

This makes the same design usable later on interfaces such as `eth0` or
`wlan0` on the Raspberry Pi.

### `capture.c`

Responsible only for packet acquisition.

Current flow:

``` text
interface name
     |
     v
if_nametoindex()
     |
     v
socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))
     |
     v
sockaddr_ll + bind()
     |
     v
recvfrom()
     |
     v
process_packet()
```

Raw packet sockets normally require root privileges or the appropriate
Linux capability.

The capture loop handles `SIGINT` so `Ctrl+C` can terminate the program
cleanly. A `volatile sig_atomic_t` flag is changed by the signal
handler, and an interrupted `recvfrom()` is handled through `EINTR`.

Because the project uses ISO C99 while `sigaction()` is a POSIX API, the
capture source enables the required POSIX declarations with a
feature-test macro before the system headers.

### `packet.c`

Coordinates parsing and dispatch. It does not decode every protocol
itself.

Conceptually:

``` text
EtherType
   |
   +-- IPv4
         |
         +-- ICMP
         +-- TCP
         +-- UDP
               |
               +-- TWAMP
```

Unsupported or currently uninteresting traffic can be ignored without
being treated as a parser failure.

### `ethernet.c`

Parses the 14-byte Ethernet II header:

``` text
Destination MAC : 6 bytes
Source MAC      : 6 bytes
EtherType       : 2 bytes
```

Relevant EtherType:

``` text
0x0800 = IPv4
```

### `ipv4.c`

Parses fields including:

-   Version
-   IHL
-   Total Length
-   TTL
-   Protocol
-   Source address
-   Destination address

The IPv4 header size is calculated from IHL:

``` c
header_length = ihl * 4;
```

This avoids assuming that every IPv4 header is exactly 20 bytes.

### `icmp.c`

Currently parses ICMP Echo traffic, including:

``` text
Type 8 = Echo Request
Type 0 = Echo Reply
```

Extracted fields include Type, Code, Identifier, and Sequence Number.

### `udp.c`

Parses the 8-byte UDP header:

``` text
Source Port      2 bytes
Destination Port 2 bytes
Length           2 bytes
Checksum         2 bytes
```

Before accessing the payload, NetAgent validates that the UDP length is
at least the header size and does not exceed the available IPv4 payload.

### `twamp.c`

The current TWAMP parser decodes the initial fixed fields used in the
Session-Sender test:

``` text
Sequence Number       4 bytes
Timestamp Seconds     4 bytes
Timestamp Fraction    4 bytes
Error Estimate        2 bytes
```

Current minimum parsed size: 14 bytes.

The TWAMP/NTP timestamp is kept as two 32-bit components:

``` text
seconds  : 32 bits
fraction : 32 bits
```

The TWAMP implementation is intentionally incremental and does not yet
model every TWAMP packet field or operating mode.

## Build

Configure:

``` bash
cmake -S . -B build/cmake
```

Build:

``` bash
cmake --build build/cmake
```

Verbose build:

``` bash
cmake --build build/cmake --verbose
```

Representative CMake configuration:

``` cmake
cmake_minimum_required(VERSION 3.20)

project(netagent LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

target_compile_options(netagent PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
```

## Running Live Capture

List interfaces:

``` bash
ip -br link
```

Run NetAgent:

``` bash
sudo ./build/cmake/netagent ens33
```

The packet socket currently uses:

``` c
socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
```

Meaning:

-   `AF_PACKET`: Linux link-layer packet socket.
-   `SOCK_RAW`: Ethernet header remains visible to the application.
-   `ETH_P_IP`: only IPv4 Ethernet frames are delivered to this socket.

Earlier development used `ETH_P_ALL`. Moving to `ETH_P_IP` prevents
traffic such as ARP and IPv6 from reaching this socket.

This does **not** yet restrict IPv4 traffic to UDP. TCP, ICMP, and UDP
can all still reach `recvfrom()`.

## Real TWAMP Capture Used for Validation

A real captured Ethernet/IPv4/UDP frame was used to validate the parsing
pipeline.

Decoded fields included:

``` text
Ethernet:
    EtherType        = 0x0800

IPv4:
    Version          = 4
    IHL              = 5
    Protocol         = 17 (UDP)
    Source           = 10.10.10.10
    Destination      = 10.10.10.233

UDP:
    Source Port      = 24003
    Destination Port = 20481

TWAMP:
    Sequence Number  = 1
    Timestamp Sec    = 0x83aa8043
    Timestamp Frac   = 0x6f863f60
    Error Estimate   = 0x0081
```

An important conclusion from this capture is that NetAgent must not
assume that TWAMP universally uses UDP port `50000`. Port `20481`
currently appears in the development capture and may be used as a
temporary filter, but the session/configuration should ultimately
determine the relevant port.

## Current Filtering Architecture

Current filtering is split between the packet socket and userspace:

``` text
NIC
 |
 v
Kernel
 |
 | ETH_P_IP
 v
AF_PACKET socket
 |
 v
recvfrom()
 |
 v
IPv4 protocol dispatch
 |
 +-- ICMP
 +-- TCP
 +-- UDP
       |
       v
   UDP port check
       |
       +-- irrelevant --> ignore
       |
       +-- relevant --> TWAMP parsing
```

The UDP protocol and port checks currently happen after the frame has
crossed from kernel space into userspace.

## Next Milestone: Classic BPF

The next planned step is to attach a classic BPF socket filter so
irrelevant IPv4 packets can be rejected before they are queued to
NetAgent.

Planned progression:

``` text
1. AF_PACKET raw capture                    DONE
2. Bind to selected interface               DONE
3. Restrict socket to IPv4 with ETH_P_IP    DONE
4. Userspace protocol/port filtering        DONE
5. Classic BPF filter for UDP               NEXT
6. BPF filtering for the relevant UDP port
7. Compare kernel filtering with userspace filtering
```

Target flow:

``` text
NIC
 |
 v
Kernel
 |
 v
AF_PACKET
 |
 v
BPF program
 |
 +---- reject irrelevant frame
 |
 +---- accept
         |
         v
      recvfrom()
         |
         v
      NetAgent parsers
```

## Longer-Term Roadmap

Planned work includes:

-   Classic BPF socket filtering.
-   Configurable UDP/TWAMP ports instead of magic numbers.
-   Stronger TWAMP validation.
-   More complete TWAMP Sender/Reflector parsing.
-   NTP timestamp conversion.
-   Packet/statistics counters.
-   TCP parsing.
-   Structured output/logging.
-   Parser unit tests.
-   PCAP-based regression tests.
-   Raspberry Pi 4 deployment.
-   x86-64 versus ARM64 build/testing.
-   Performance measurements.
-   Later exploration of mechanisms such as `PACKET_MMAP` and eBPF after
    the fundamentals are established.

## Development Environment

Initial VM environment:

``` text
OS:       Debian GNU/Linux 13 (trixie)
Kernel:   Linux 6.12.x
Arch:     x86_64
Compiler: GCC 14.2
Build:    CMake
NIC:      ens33
Driver:   e1000
```

## Learning Goals

NetAgent is intended to provide practical experience with:

-   C systems programming
-   Linux networking
-   Ethernet
-   IPv4
-   ICMP
-   UDP
-   TWAMP
-   Network byte order and endianness
-   Pointer/buffer safety
-   Linux sockets and raw sockets
-   File descriptors
-   POSIX APIs
-   Signals
-   Kernel/userspace boundaries
-   BPF packet filtering
-   CMake and Git
-   Debugging real network traffic

The long-term objective is not merely to build a sniffer. It is to
understand what happens to a network frame from arrival at the machine,
through the Linux networking path, until userspace software receives,
validates, parses, filters, and acts on it.

My command:
cmake -S . -B build/cmake && cmake --build build/cmake && sudo build/cmake/netagent ens33