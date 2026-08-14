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

## libbattleroyale
> flexible transport layer for games

libbattleroyale is our implementation of libssh. It provides the transport layer support for our games, by handling both data transmission and security.
* **Message Format:** A simple messaging protocol was created for the passing of messages between server and clients. Simple message codes allow for the server to communicate key changes in the life cycle of the game.
* **TCP and UDP:** We provide flexibility, offering both transport layer protocols as options for data transmission. TCP can be used for all important information which requires guarenteed delivery, while UDP can be used for speedy sending of non-essential updates.
* **Authentication:** Our clients and server perform a simple authentication handshake upon starting the connection. The server has a signed certificate which the client verifies.
* **Encryption:** All unicast communication, UDP or TCP is encrypted using AES-256. Keys are generated and passed by the client to the server using RSA.
    * Broadcasts remain enencrypted, as they are public and expected to be seen by multiple clients, rendering symmetric encryption unecessary.

### Authoritative Server
Our library offers developers a suite of functions which can be integrated seamlessly into a game server or client's codebase. They may interact with our library with a variety of asynchronous function calls.
* We design around an authoritative server, where all information must be passed to and evaluated by the server. When the server processes a change in state, its state should be taken as absolute truth by the client.
* Clients never directly interact with one another, but they can be made aware of one another if updated by the server.

### Background Thread
To ensure that the operation of the server or client does not block primary function of developer applications, a worker thread is spawned, which mainly functions as a listener.
* The server uses the background thread to accept connections and listen to developer made application layer messages.
* The client uses the background thread to listen for server updates, those for libbattleroyale and application layer purposes.

## libhypertext and libhtttp
A library to help implement HTTP-alike protocols, providing tools like parsers and standardised structs which emulate HTTP formatting. It was designed as 2 libraries, with libhtttp implementing libhypertext.
* When designing the library, libhypertext was intended as a generic so that tetris and bomberman could have their own independent protocols
* libhtttp implemented with methods and header specification specific to tetrish. Also included are functions for encoding and decoding specific payloads. This is because a pre-existing protocol used for early testing of tetris was adapted into the htttp.

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

------------------------------------------------
libtetrisbrain:

1. The Pure State Machine (No I/O)
The core design philosophy of the brain is a decoupled, pure state machine. The library is entirely unaware of system time, networking, or the terminal display.

Time Abstraction: 
Real-time gravity and lock delays are decoupled from the OS. Instead of using sleep() or system clocks, time is abstracted into integer "ticks" (gravity_timer and lock_timer) advanced externally by the daemon.

Dependency Injection for Networking:
The engine handles Battle Royale targeting without touching network sockets. A `Roster` struct is passed in as a read-only parameter, allowing the game logic to resolve targets based purely on provided inputs.

2. Memory & Computational Efficiency
To ensure the library can scale when `tetrisd` runs multiple concurrent rooms, the data structures were optimized for low memory footprint and O(1) operations.

Mathematical Rotation: Instead of storing massive 3D arrays for every rotated state of every piece, tetrominoes are stored as a single, read-only 1D array of 16 integers. Rotation states are calculated on the fly using O(1) coordinate translation.

Static SRS Tables:
The Super Rotation System (SRS) utilizes read-only static transition tables for wall kicks, minimizing runtime allocations.

3. Battle Royale Mechanics
The multiplayer interactions follow modern competitive guidelines (like TETR.IO) to ensure balanced gameplay.

Garbage Cancellation: 
Incoming garbage is explicitly buffered in pending_garbage. When a player clears lines, the outgoing damage cancels out the pending damage before applying to the board or sending to an opponent.

Algorithmic Combos:
Garbage generation utilizes a static lookup table (`COMBO_TABLE`) to apply non-linear scaling for sustained combos without infinite power creep.

4. Testing & Reliability
To validate this state-driven architecture, applying custom fuzzers for memory safety and state consistency testing directly to the engine components ensures bounds checks hold up under extreme inputs. For example, fuzzing `add_garbage` guarantees that malicious or corrupted incoming line counts cannot trigger a buffer overflow when the board array shifts.

------------------------------------------------

tetrisu:

1. The Authoritative "Thin Client" Model
To guarantee competitive integrity in the Battle Royale mode, the client does not compute its own game state.

Action Streaming:
Keystrokes are parsed into abstract payloads (e.g., `ACTION_MOVE_LEFT`) and shipped immediately to the server.


State Application:
The client only updates its visual board when it receives an authoritative PACKET_STATE frame pushed by the daemon. This strict separation prevents local memory manipulation or modified clients from cheating, and ensures the client remains resilient even if exposed to malformed state packets during memory safety testing or fuzzing.

2. Non-Blocking Concurrency
The client flawlessly handles the I/O multiplexing problem (reading the network and keyboard simultaneously) without relying on heavy multithreading.

File Descriptor Manipulation:
Keyboard input is handled using standard POSIX fcntl sys-calls to temporarily apply the O_NONBLOCK flag to STDIN_FILENO. If no key is ready, kbhit() returns instantly, allowing the main loop to immediately check the network message queue via brclient_get_app_msg and render the next frame.

3. Graceful Degradation & Signal Handling
The terminal is pushed into "raw mode" by modifying POSIX termios flags, disabling canonical input (ICANON) and character echoing (ECHO) to allow real-time keystroke streaming.

State Restoration:
The original terminal state is saved at startup. An atexit(disable_raw_mode) callback is registered to guarantee the terminal resets to its normal state upon a clean exit (e.g., pressing q ).

Signal Interception: 
To handle abrupt termination, a custom POSIX signal handler intercepts SIGINT (Ctrl+C), calling exit(0) to cleanly route the abnormal termination through the atexit cleanup hook, preventing a "bricked" terminal.

------------------------------------------------
tetrislogd & IPC:

1. IPC Mechanism: Unix Domain Datagram Sockets
To satisfy the production realism requirement, the logging system utilizes a Unix Domain Socket (AF_UNIX) running in Datagram mode (SOCK_DGRAM).

Message Boundaries:
Unlike stream sockets or named pipes, datagrams natively preserve packet boundaries. A single 292-byte sendto() guarantees exactly one 292-byte recvfrom(), eliminating the need for complex length-prefixed framing.

Failure Isolation: 
Because datagram sockets are connectionless, if the logger daemon crashes, the game daemon does not break. The game simply fires packets at a dead socket file, which are harmlessly dropped without halting the game thread.

2. Client-Side Resilience (Asynchronous Logging)
The client library prioritizes the game server's tick rate over immediate log delivery.

The Ring Buffer:
When the game generates a log, it is not transmitted immediately. Instead, it is pushed into an O(1) local memory ring buffer.

Non-Blocking Transmissions:** The socket is explicitly set to  O_NONBLOCK. During the game loop's drain phase, a bounded batch of records (maximum of 16) is transmitted. If the OS buffer is full or the daemon is dead, sendto() returns an error instantly rather than blocking the thread, successfully isolating the failure domains.

3. Daemon Concurrency & Signal Handling
The logger daemon is designed to be highly responsive to POSIX signals while remaining perfectly memory-safe.

Asynchronous-Signal-Safe Flags:
SIGTERM (shutdown) and SIGHUP (log rotation) are intercepted using custom handlers that only toggle volatile sig_atomic_t flags. This prevents catastrophic race conditions (like calling fclose() mid-write) by deferring all file operations to the main thread's safe execution path.

The select() Event Loop:
Instead of a blocking recvfrom() that would freeze the daemon indefinitely, the event loop uses select() with a 1-second timeout. This guarantees CPU usage remains near 0% while idle, but wakes up instantly when a packet arrives or periodically to process signals and print dropped-record metrics.

4. Data Integrity & Gap Tracking
The protocol applies defensive systems programming techniques to ensure data is safely packed, parsed, and tracked.

Memory Safety: 
The copy_fixed helper strictly limits string manipulation using strncpy with an enforced \0 termination, immunizing the daemon against buffer overflows from maliciously oversized payloads. Endian translation (htonl / ntohl) ensures the binary wire format is strictly standardized.

Thread-Safe Time:
Log formatting utilizes the reentrant localtime_r instead of the standard localtime to prevent race conditions within the C library's global time structures.

PID-Aware Sequence Tracking:
Because multiple processes might write to the socket simultaneously, sequence gaps are calculated on a per-PID basis using a tracking table. This prevents false gap detection when two different binaries interleave their log packets.