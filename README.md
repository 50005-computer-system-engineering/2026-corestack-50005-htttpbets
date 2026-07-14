# CoreStack: TetriSH Battle Royale 🎮

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Network](https://img.shields.io/badge/Network-TCP%2FUDP-green.svg)
![Architecture](https://img.shields.io/badge/Architecture-Distributed-purple.svg)

**CoreStack** is a comprehensive distributed multi-component architecture powering a Bomberman Battle Royale-styled Tetris game, modeled after Jackbox's host-player system. This project is developed as part of the **50.005 Computer System Engineering** course.

## How 2 Run (temp):
`make bomberman` or `make tetris`

Then run `./bomberman/launcher` or `./tetris/launcher`

## 👥 Team HTTTPBets (C1C7)

| Student ID | Full Name | Role |
| :--- | :--- | :--- |
| 1009367 | Ho Li Lian | Systems |
| 1009307 | Ryan Ngo Shen Kai | Networking & Security |
| 1009264 | Ng Sheng Qi Ethan | Application and Integration |

---

## 🏗️ Architecture Overview

The system is composed of **5 binaries** and **3 static libraries**, extended with a RayLib-rendered client and a **dual-transport (TCP + UDP)** network layer.

### Binaries

| Binary | Description |
| :--- | :--- |
| `tetrish` | Interactive shell; reads `tetrishrc` on startup, spawns daemons via `dspawn`, provides REPL with all PA1 builtins. |
| `tetrisd` | Concurrent game server process; hosts game rooms, manages player sessions, drives game ticks via `libtetrisbrain`. |
| `tetrislogd` | Dedicated logger daemon; receives log records over IPC and writes them to disk. |
| `tetrisctl` | Admin CLI; communicates with `tetrisd` over a local Unix domain socket control plane. |
| `tetrisu` | Game client rendered with RayLib; connects via both TCP and UDP. |

### Static Libraries

| Library | Description |
| :--- | :--- |
| `libtetrissh` | Secure session library: certificate auth, RSA-wrapped AES key exchange, framed encryption over TCP. |
| `libhtttp` | HTTTP protocol parser and serialiser; linked into both `tetrisd` and `tetrisu` to prevent protocol drift. |
| `libtetrisbrain` | Pure Tetris game logic (pieces, rotation, gravity, line clear, scoring). |

---

## 🌐 Network Layer: Dual-Transport Design

Our network design leverages the strengths of both TCP and UDP to deliver a responsive, secure, and accurate multiplayer experience:

* **TCP (Secure, Reliable, State-Changing):** Carries all state-changing and reliable messages (JOIN, LEAVE, Placed Blocks, Line Clears, STATE frames). All TCP traffic is securely wrapped by `libtetrissh` using AES-256 session encryption.
* **UDP (High-Frequency, Loss-Tolerant):** Handles live position updates of falling pieces across player screens and cosmetic broadcasts (e.g., score, "T-SPIN!"). Packets are raw POSIX `SOCK_DGRAM`. 
    * *Note: UDP data is used strictly for display interpolation. The server remains fully authoritative for game logic.*

---

## ⚡ Concurrency Model

Built for scale, the system uses an **epoll-based event loop + dedicated auxiliary threads** to support 200+ concurrent clients efficiently.

| Thread / Process | Function | Blocking Syscall / Invariant |
| :--- | :--- | :--- |
| **epoll main loop** | Multiplexes all TCP client fds, TCP listener fd, control plane socket fd, and timefd-based room ticks. | `epoll_wait()` |
| **UDP Listener** | Receives position packets, updates per-room live buffers, and rebroadcasts via `sendto()`. | `recvfrom()` |
| **Per-room ticker** | Runs game ticks (20Hz), advances game state via `libtetrisbrain`, enqueues STATE broadcasts. | `nanosleep()` / `timerfd` |
| **Log-shipper** | Wakes up to drain the in-process ring buffer and forwards to `tetrislogd` via POSIX MQ. | `pthread_cond_wait()` |
| **Signal handler** | Manages SIGTERM, SIGHUP, SIGUSR1. Avoids async-signal-safety concerns across the codebase. | `sigwaitinfo()` |

*Concurrency Invariants:* * No mutex is held across a blocking syscall. 
* Global lock acquisition order is strictly enforced: `per-room mutex` -> `live-positions buffer lock`.

---

## 🔄 IPC Mechanisms (Local-Only)

We leverage robust POSIX standard IPC mechanisms for high-performance intra-system communication:

1.  **POSIX Message Queue (`tetrisd` ↔ `tetrislogd`):** Used for fast, non-blocking log shipping. A dedicated log-shipper thread pushes data via `mq_send()`.
2.  **Unix Domain Socket (`tetrisctl` ↔ `tetrisd`):** A control plane stream socket (e.g., `/var/run/tetrish/ctl_sock`) allowing admin commands using the HTTTP wire format without blocking game traffic.
3.  **Shared Memory + Semaphore (Battle Royale Garbage Pool):** A fixed-sized ring buffer in shared memory holds pending garbage events. Target room tickers wait on semaphores to inject garbage rows securely under their room mutex.

---

## 🚀 End-to-End Hard Drop Flow Example
1. **Client (`tetrisu`)** sends `DROP HARD` over encrypted TCP.
2. **Server (`tetrisd`)** epoll loop reads, decrypts, parses, and dispatches to the room handler.
3. **Room Handler** locks the piece, checks line clears via `libtetrisbrain` under the room mutex.
4. **Room Ticker** sends the updated `STATE` frame to all connected players.
5. If $\ge2$ lines are cleared, a garbage event is written to the SHM ring buffer, triggering a semaphore for the target room.
6. The entire event is asynchronously logged to disk via `tetrislogd`.

---
## 📦 Repositories & References
* **WIP Corestack:** [Corestack GitHub Repo](https://github.com/50005-computer-system-engineering/2026-corestack-50005-htttpbets)
* **Completed PA1 & PA2:** [PA1 Repo](https://github.com/50005-computer-system-engineering/2026-pa1-50005-htttpbets) | [PA2 Repo](https://github.com/50005-computer-system-engineering/2026-pa2-50005-htttpbets)