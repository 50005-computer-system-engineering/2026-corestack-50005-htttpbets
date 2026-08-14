#include "bombfuzzer_common.h"
#include "lib/libhypertext.h"
#include "lib/libhtttp/payload.h"
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>

// per-thread crash guard: a bug in the code under test (eg. convert_to_hypertext's OOB read) can
// segfault a whole thread with no isolation otherwise - fork-per-op like the other harnesses use
// would defeat the point of a throughput test, so this catches SIGSEGV/SIGBUS in-process instead
// and treats it as a failed round trip. __thread makes the jump target and guard flag distinct per
// thread; the handler is process-wide (POSIX signal handlers always are) but only acts if the
// specific thread it interrupted had actually armed its own guard.
static __thread sigjmp_buf crashJmpBuf;
static __thread volatile sig_atomic_t crashArmed = 0;

static void crashHandler(int sig)
{
    (void)sig;
    if (crashArmed)
    {
        siglongjmp(crashJmpBuf, 1);
    }
}

// Deliberately NOT #include "lib/libhtttp.h" - see bombfuzzer_htttp.c's top comment for why
// (multiple-definition linker bug). Declaring only what we call routes around it.
typedef enum { UNKNOWN = 0, REQ_MOVE, REQ_DROP, REQ_ROTATE, REQ_STATE, REQ_ATTACK } MethodHTTTP;
void req_create_action(uint32_t id, MethodHTTTP method, InputPayload *payload, ParsedMsgHT *formatted_msg);

// concurrency/volume stress: hammers the pure parsing/encoding functions from many threads at
// once. Each thread checks its own round trips stay correct despite everyone else running at the
// same time - this would catch hidden shared state, since none of these functions are supposed to
// have any. Also reports rough throughput. A local precursor to real load testing, not a
// replacement for JMeter against a live server - that still needs bombd.

#define THREADS_DEFAULT 16
#define ITERS_PER_THREAD_DEFAULT 500

static int threadsCount(void)
{
    const char *env = getenv("BOMBFUZZER_STRESS_THREADS");
    return env != NULL ? atoi(env) : THREADS_DEFAULT;
}

static int itersPerThread(void)
{
    const char *env = getenv("BOMBFUZZER_STRESS_ITERS");
    return env != NULL ? atoi(env) : ITERS_PER_THREAD_DEFAULT;
}

typedef struct {
    int threadIdx;
    int iters;
    int mismatches;
} ThreadArg;

// one unit of work: build+round-trip a request and every payload type using values unique to
// (threadIdx, iter), so any cross-thread interference shows up as a mismatch. Returns 1 if every
// round trip matched, 0 otherwise.
static int roundTripOnce(int threadIdx, int iter)
{
    if (sigsetjmp(crashJmpBuf, 1) != 0)
    {
        crashArmed = 0;
        return 0;   // landed here via crashHandler - treat the crash itself as a failed round trip
    }
    crashArmed = 1;

    uint32_t id = (uint32_t)(threadIdx * 1000000 + iter);

    // hypertext request round trip
    InputPayload input = {.action = id % 8};
    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    req_create_action(id, REQ_MOVE, &input, &msg);

    HyperText ht;
    if (convert_to_hypertext(&msg, ht) < 0)
    {
        return 0;
    }
    ParsedMsgHT parsed;
    memset(&parsed, 0, sizeof(parsed));
    parse_hypertext(ht, &parsed);
    // checked directly rather than via req_extract_info(), which is currently a no-op stub - see
    // bombfuzzer_htttp.c's case_req_extract_info_correctness
    char expectedPath[64];
    snprintf(expectedPath, sizeof(expectedPath), "/room/0/player/%u", id);
    if (parsed.token1 == NULL || strcmp(parsed.token1, "MOVE") != 0 || parsed.token2 == NULL ||
        strcmp(parsed.token2, expectedPath) != 0)
    {
        return 0;
    }

    // attack payload round trip
    AttackPayload attack = {.source_player = id, .target_player = id + 1, .lines = id % 4};
    char attackBuf[MAX_BUF];
    payload_encode_attack(attackBuf, &attack);
    AttackPayload attackOut;
    memset(&attackOut, 0, sizeof(attackOut));
    payload_decode_attack(attackBuf, &attackOut);
    if (attackOut.source_player != attack.source_player || attackOut.target_player != attack.target_player ||
        attackOut.lines != attack.lines)
    {
        return 0;
    }

    // roster payload round trip
    unsigned char rosterBuf[sizeof(RosterPayload) + 2 * sizeof(uint32_t)];
    RosterPayload *roster = (RosterPayload *)rosterBuf;
    roster->count = 2;
    roster->ids[0] = id;
    roster->ids[1] = id + 1;
    char rosterEncoded[MAX_BUF];
    payload_encode_roster(rosterEncoded, roster);
    unsigned char rosterOutBuf[sizeof(RosterPayload) + 2 * sizeof(uint32_t)];
    RosterPayload *rosterOut = (RosterPayload *)rosterOutBuf;
    memset(rosterOut, 0, sizeof(rosterOutBuf));
    payload_decode_roster(rosterEncoded, rosterOut);
    if (rosterOut->count != roster->count || rosterOut->ids[0] != roster->ids[0] || rosterOut->ids[1] != roster->ids[1])
    {
        return 0;
    }

    // state payload round trip (a couple representative fields, not the full board)
    StatePayload state;
    memset(&state, 0, sizeof(state));
    state.player_id = id;
    state.score = id * 2;
    char stateEncoded[MAX_BUF];
    payload_encode_state(stateEncoded, &state);
    StatePayload stateOut;
    memset(&stateOut, 0, sizeof(stateOut));
    payload_decode_state(stateEncoded, &stateOut);
    if (stateOut.player_id != state.player_id || stateOut.score != state.score)
    {
        return 0;
    }

    return 1;
}

static void *threadMain(void *arg)
{
    ThreadArg *t = (ThreadArg *)arg;
    for (int i = 0; i < t->iters; i++)
    {
        int ok = roundTripOnce(t->threadIdx, i);
        crashArmed = 0;   // disarm here - covers both normal return and the longjmp-recovery path
        if (!ok)
        {
            t->mismatches++;
        }
    }
    return NULL;
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = crashHandler;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);

    unsigned int seed = bombfuzzer_init_seed();

    int nThreads = threadsCount();
    int iters = itersPerThread();
    printf("bombfuzzer_stress: %d threads x %d iterations each\n", nThreads, iters);

    pthread_t *tids = malloc(sizeof(pthread_t) * (size_t)nThreads);
    ThreadArg *args = malloc(sizeof(ThreadArg) * (size_t)nThreads);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < nThreads; i++)
    {
        args[i] = (ThreadArg){.threadIdx = i, .iters = iters, .mismatches = 0};
        pthread_create(&tids[i], NULL, threadMain, &args[i]);
    }

    int totalMismatches = 0;
    for (int i = 0; i < nThreads; i++)
    {
        pthread_join(tids[i], NULL);
        totalMismatches += args[i].mismatches;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    long totalOps = (long)nThreads * iters;

    printf("bombfuzzer_stress: %ld round trips across %d threads in %.2fs (%.0f ops/sec)\n", totalOps, nThreads,
           elapsed, elapsed > 0 ? (double)totalOps / elapsed : 0.0);

    char actual[128];
    snprintf(actual, sizeof(actual), "%d of %ld round trips came back mismatched under concurrent execution",
             totalMismatches, totalOps);
    bombfuzzer_report(totalMismatches == 0, "bombfuzzer_stress", "concurrent round-trip consistency", seed, NULL, 0,
                       "every thread's own request/payload round trips stay correct under concurrent execution",
                       actual);

    free(tids);
    free(args);

    printf("bombfuzzer_stress: done (%d mismatches)\n", totalMismatches);
    return 0;
}
