#ifndef BOMBFUZZER_COMMON_H
#define BOMBFUZZER_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <time.h>

// bombfuzzer: shared infrastructure for the bombfuzzer_*.c harnesses (docs/THREAT_MODEL.md 2.4).
// bombd doesn't exist yet, so this targets the shared corestack libraries it will be built on:
// libbattleroyale (raw framing), libhypertext (text protocol), libhtttp (methods/payloads).
//
// Layers, bottom-up:
//   bombfuzzer_bytes.c     - read_bytes/send_bytes over a socketpair (libbattleroyale)
//   bombfuzzer_message.c   - receive_message_tcp/send_message_tcp (libbattleroyale)
//   bombfuzzer_hypertext.c - parse_hypertext/msg_add_header/convert_to_hypertext (libhypertext),
//                            plus mutation-based fuzzing of a well-formed request
//   bombfuzzer_htttp.c     - payload_encode/decode_*, req_create_*/req_extract_info (libhtttp),
//                            plus mutation-based fuzzing of each payload type
//   bombfuzzer_stress.c    - many-threaded round-trip volume/concurrency check across all layers

#define BOMBFUZZER_TIMEOUT_SEC 8                            // per-case hang timeout; 3s gave false positives under load
#define BOMBFUZZER_FINDINGS_DIR "tests/bombfuzzer/findings" // relative to bomberman/, see Makefile

// a single fuzz finding: id, precondition, input, expected vs actual outcome
typedef struct {
    const char* harness;        // member (which bombfuzzer_*.c layer raised this)
    const char* description;    // member (equivalence class / mutation that produced the input)
    unsigned int seed;          // member (PRNG seed - rerun with BOMBFUZZER_SEED=<seed> to reproduce)
    const unsigned char* input; // member (raw bytes fed to the target function, NULL if not applicable)
    uint64_t input_len;         // member (length of input, in bytes)
    const char* expected;       // member (what the target under test should have done)
    const char* actual;         // member (what actually happened - crash/hang/wrong result)
} BombfuzzerFinding;

// jmp target used by the SIGALRM hang guard, see bombfuzzer_arm_timeout below
extern sigjmp_buf bombfuzzer_jmpbuf;

// PRNG functions - seeded once per run via bombfuzzer_init_seed()
unsigned int bombfuzzer_init_seed(void);
uint8_t bombfuzzer_rand_byte(void);
uint32_t bombfuzzer_rand_u32(void);
uint64_t bombfuzzer_rand_u64(void);

// mutation helper - bit-flip a byte in a known-valid buffer
void bombfuzzer_flip_random_bit(unsigned char* buf, uint64_t len);

// hang guard - wrap a fuzz case:
//   if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0) { bombfuzzer_arm_timeout(); ...call...; bombfuzzer_disarm_timeout(); }
//   else { /* hung past BOMBFUZZER_TIMEOUT_SEC */ }
void bombfuzzer_arm_timeout(void);
void bombfuzzer_disarm_timeout(void);
void bombfuzzer_arm_timeout_secs(unsigned int secs); // custom duration for a longer-running case

void bombfuzzer_log_finding(const BombfuzzerFinding* finding);

// logs a finding only when ok == 0 - what fuzz cases call after each attempt
void bombfuzzer_report(int ok, const char* harness, const char* description, unsigned int seed,
                       const unsigned char* input, uint64_t input_len,
                       const char* expected, const char* actual);

// runs fn(arg) in a forked child, for cases expected to crash rather than hang. Returns 0 if it
// exited cleanly, the signal number if killed by one, or -1 if force-killed after timeoutSecs.
typedef void (*BombfuzzerCaseFn)(void* arg);
int bombfuzzer_run_isolated(BombfuzzerCaseFn fn, void* arg, unsigned int timeoutSecs);

#endif
