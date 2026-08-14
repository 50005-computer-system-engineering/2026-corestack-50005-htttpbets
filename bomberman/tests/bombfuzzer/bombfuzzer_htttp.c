#include "bombfuzzer_common.h"
#include "lib/libhtttp.h"
#include <string.h>

// fuzzes libhtttp - the method/payload layer built on top of libhypertext (payload_encode/
// decode_*, req_create_*, req_extract_info). Bodies here are plain sscanf/sprintf text produced
// by an untrusted peer.

// low default: these cases use fixed input, so repeats add no coverage - see
// bombfuzzer_hypertext.c's ITER_DEFAULT comment
#define ITER_DEFAULT 5
#define ROSTER_CAPACITY 4   // mimics a caller who sized ids[] for a small, expected lobby

static int iterations(void)
{
    const char *env = getenv("BOMBFUZZER_ITERS");
    return env != NULL ? atoi(env) : ITER_DEFAULT;
}

static RosterPayload *allocRoster(uint32_t capacity)
{
    return malloc(sizeof(RosterPayload) + (size_t)capacity * sizeof(uint32_t));
}

static void reportCrashProbe(const char *description, unsigned int seed, const char *buf, int sig,
                              const char *expected)
{
    char actual[128];
    snprintf(actual, sizeof(actual), sig > 0 ? "crashed with signal %d" : "hung past the timeout", sig);
    bombfuzzer_report(sig == 0, "bombfuzzer_htttp", description, seed, (const unsigned char *)buf, strlen(buf),
                       expected, actual);
}

// class 1: roster body claims more ids than the caller actually allocated room for -
// payload_decode_roster() has no idea how big payload->ids[] really is, it just trusts `count`
// straight out of the (attacker-controlled) text and writes that many entries
typedef struct {
    char buf[256];
} RosterArg;

static void doDecodeRosterOverflow(void *arg)
{
    RosterPayload *payload = allocRoster(ROSTER_CAPACITY);
    payload_decode_roster(((RosterArg *)arg)->buf, payload);
    free(payload);
}

static void case_roster_count_overflow(unsigned int seed)
{
    RosterArg arg;
    // claims 64 ids (far past ROSTER_CAPACITY=4) but only actually lists 2
    snprintf(arg.buf, sizeof(arg.buf), "{count: 64, ids: [1 2 ]}");

    int sig = bombfuzzer_run_isolated(doDecodeRosterOverflow, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("roster count exceeds allocated capacity", seed, arg.buf, sig,
                      "payload_decode_roster stays within the caller's allocated ids[] capacity");
}

// class 2: no "ids: [" marker at all - strstr() returns NULL, and NULL + strlen("ids: [") is
// added to it unchecked, handing sscanf() a bogus low address to read from next iteration
static void doDecodeRosterNoMarker(void *arg)
{
    RosterPayload *payload = allocRoster(ROSTER_CAPACITY);
    payload_decode_roster(((RosterArg *)arg)->buf, payload);
    free(payload);
}

static void case_roster_missing_marker(unsigned int seed)
{
    RosterArg arg;
    snprintf(arg.buf, sizeof(arg.buf), "{count: 2, nothing resembling the expected marker here}");

    int sig = bombfuzzer_run_isolated(doDecodeRosterNoMarker, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("roster missing \"ids: [\" marker", seed, arg.buf, sig,
                      "payload_decode_roster rejects a body with no ids marker instead of crashing");
}

// class 3: payload_decode_state() unpacks a fixed 200 board bytes via buffer[offset++] with no
// length check at all - a short buffer means it reads straight past the end of the allocation
typedef struct {
    char buf[16];
} StateArg;

static void doDecodeStateShort(void *arg)
{
    StatePayload payload;
    memset(&payload, 0, sizeof(payload));
    payload_decode_state(((StateArg *)arg)->buf, &payload);
}

static void case_state_short_buffer(unsigned int seed)
{
    StateArg arg;
    snprintf(arg.buf, sizeof(arg.buf), "short");   // nowhere near the 200 board bytes expected

    int sig = bombfuzzer_run_isolated(doDecodeStateShort, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("state decode with short buffer", seed, arg.buf, sig,
                      "payload_decode_state validates buffer length before unpacking 200 board bytes");
}

// class 4: garbage/non-numeric bodies fed to the simpler sscanf-based decoders - these don't index
// arrays so shouldn't corrupt memory, but sscanf's return value is never checked either, so a
// malformed body silently leaves the output struct partially (or fully) uninitialised
static void doDecodeGarbage(void *arg)
{
    const char *garbage = (const char *)arg;
    AttackPayload attack;
    memset(&attack, 0, sizeof(attack));
    payload_decode_attack(garbage, &attack);

    InputPayload input;
    memset(&input, 0, sizeof(input));
    payload_decode_input(garbage, &input);
}

static void case_garbage_attack_input(unsigned int seed)
{
    const char *garbage = "definitely not the expected {source-player: ...} format";

    int sig = bombfuzzer_run_isolated(doDecodeGarbage, (void *)garbage, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("garbage attack/input body", seed, garbage, sig,
                      "payload_decode_attack/input do not crash on a malformed body (sscanf just "
                      "leaves fields unset - callers should zero-init before decoding)");
}

// class 5: req_extract_info() should recover the same method/Player-Id/body that
// req_create_action() built
static void case_req_extract_info_correctness(unsigned int seed)
{
    InputPayload payload = {.action = 7};
    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    req_create_action(42, REQ_MOVE, &payload, &msg);

    MethodHTTTP method = UNKNOWN;
    uint32_t id = 0;
    char *body = NULL;
    req_extract_info(&msg, &method, &id, &body);

    int correct = (method == REQ_MOVE) && (id == 42) && (body != NULL) && (strstr(body, "action-id: 7") != NULL);
    bombfuzzer_report(correct, "bombfuzzer_htttp", "req_extract_info correctness", seed, NULL, 0,
                       "req_extract_info recovers the same method/Player-Id/body req_create_action built",
                       "req_extract_info returned a method, id, or body that didn't match what was built");
}

// class 6: req_create_action/state/attack all try to attach a "Date" header via get_date_str(),
// whose format ("%a, %d %b %Y %H:%M:%S GMT") contains ':' - but msg_add_header() rejects ':' in
// the VALUE too, not just the field name, and none of the three callers check its return value.
// The Date header is therefore silently dropped from every request built this way.
static void case_date_header_silently_dropped(unsigned int seed)
{
    InputPayload payload = {.action = 1};
    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    req_create_action(1, REQ_MOVE, &payload, &msg);

    int hasDate = 0;
    for (int i = 0; i < msg.n_headers; i++)
    {
        if (msg.headers[i].field != NULL && strcmp(msg.headers[i].field, "Date") == 0)
        {
            hasDate = 1;
        }
    }

    bombfuzzer_report(hasDate, "bombfuzzer_htttp", "Date header silently dropped", seed, NULL, 0,
                       "req_create_action's request carries the Date header it tried to attach",
                       "Date header missing - get_date_str()'s value contains ':', which "
                       "msg_add_header() rejects, and req_create_action never checks the return code");
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    unsigned int seed = bombfuzzer_init_seed();

    printf("bombfuzzer_htttp: running deterministic one-shot cases\n");
    for (int i = 0; i < 3; i++)
    {
        unsigned int caseSeed = seed + (unsigned int)i;
        case_req_extract_info_correctness(caseSeed);
        case_date_header_silently_dropped(caseSeed);
    }

    int n = iterations();
    printf("bombfuzzer_htttp: running %d cases per remaining equivalence class\n", n);
    for (int i = 0; i < n; i++)
    {
        unsigned int caseSeed = seed + (unsigned int)i;
        case_roster_count_overflow(caseSeed);
        case_roster_missing_marker(caseSeed);
        case_state_short_buffer(caseSeed);
        case_garbage_attack_input(caseSeed);
    }

    printf("bombfuzzer_htttp: done\n");
    return 0;
}
