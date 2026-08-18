#include "bombfuzzer_common.h"
#include "lib/libhypertext.h"
#include <string.h>

// fuzzes libhypertext, the HTTP-like text protocol layered on top of libbattleroyale's raw framing.
//
// Wire format (HyperText = char[1024]): "TOKEN1 TOKEN2 TOKEN3\r\nfield: value\r\n...\r\n\r\nbody"
//
// Some cases below are expected to crash the process (SIGSEGV) - that's the finding. They run
// isolated via bombfuzzer_run_isolated() since sigsetjmp/alarm can't catch a real segfault.

// low default: most cases use fixed input, so repeats add no coverage, and libhypertext's LOG_D/
// LOG_I write unbuffered to stderr on every parse step, which gets slow in volume
#define ITER_DEFAULT 5
// class 9 (mutation-based) is genuinely randomised, so it gets its own, larger iteration count.
// Kept modest (not larger still): each case forks via bombfuzzer_run_isolated, and fork() alone
// measured ~0.5s in this environment - the Valgrind pass multiplies that further
#define MUTATE_ITER_DEFAULT 15

static int iterations(void)
{
    const char* env = getenv("BOMBFUZZER_ITERS");
    return env != NULL ? atoi(env) : ITER_DEFAULT;
}

static int mutateIterations(void)
{
    const char* env = getenv("BOMBFUZZER_MUTATE_ITERS");
    return env != NULL ? atoi(env) : MUTATE_ITER_DEFAULT;
}

// class 1: well-formed input, hand-built (not via convert_to_hypertext, which has its own bug -
// see case_convert_few_headers) so this only tests parse_hypertext itself
static void case_wellformed_roundtrip(unsigned int seed)
{
    char raw[256];
    snprintf(raw, sizeof(raw), "MOVE /room/0/player/1 HTTTP/1.0\r\nPlayer-Id: 1\r\nContent-Length: 4\r\n\r\ntest");

    HyperText ht;
    memset(ht, 0, sizeof(ht));
    memcpy(ht, raw, strlen(raw) + 1);

    ParsedMsgHT result;
    memset(&result, 0, sizeof(result));
    int rc = parse_hypertext(ht, &result);

    int fieldsOk = result.token1 != NULL && strcmp(result.token1, "MOVE") == 0 &&
                   result.token2 != NULL && strcmp(result.token2, "/room/0/player/1") == 0 &&
                   result.token3 != NULL && strcmp(result.token3, "HTTTP/1.0") == 0 &&
                   result.n_headers == 2 && result.body != NULL && strcmp(result.body, "test") == 0;
    bombfuzzer_report(fieldsOk, "bombfuzzer_hypertext", "well-formed roundtrip fields", seed,
                      (const unsigned char*)raw, strlen(raw),
                      "parse_hypertext correctly splits method/path/version/headers/body",
                      "parse_hypertext produced mismatched fields on well-formed input");

    // parse_hypertext has no reachable 'return 0' - every path falls through to 'return -1'
    bombfuzzer_report(rc == 0, "bombfuzzer_hypertext", "parse_hypertext return code", seed, NULL, 0,
                      "parse_hypertext returns 0 once it has successfully parsed the input",
                      "parse_hypertext returned -1 despite parsing successfully (dead success path)");
}

typedef struct {
    HyperText ht;
} ParseArg;

static void runParse(void* arg)
{
    ParsedMsgHT result;
    memset(&result, 0, sizeof(result));
    parse_hypertext(((ParseArg*)arg)->ht, &result);
}

static void reportCrashProbe(const char* description, unsigned int seed, const char* ht, int sig)
{
    char actual[128];
    if (sig > 0) {
        snprintf(actual, sizeof(actual), "crashed with signal %d", sig);
    } else {
        snprintf(actual, sizeof(actual), "hung past the timeout");
    }
    bombfuzzer_report(sig == 0, "bombfuzzer_hypertext", description, seed, (const unsigned char*)ht, strlen(ht),
                      "rejects malformed input gracefully instead of crashing or hanging", actual);
}

// class 2: request line has no separators - next_token() returns NULL, then gets passed straight
// back into the next next_token() call: strstr(NULL, ...)
static void case_missing_token_separator(unsigned int seed)
{
    ParseArg arg;
    memset(arg.ht, 0, sizeof(arg.ht));
    snprintf(arg.ht, sizeof(arg.ht), "MOVEnoSpaceAnywhereInThisLineAtAll");

    int sig = bombfuzzer_run_isolated(runParse, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("missing token separator", seed, arg.ht, sig);
}

// class 3: a header line with no ": " - same NULL-propagation crash, one level deeper in parsing
static void case_missing_header_separator(unsigned int seed)
{
    ParseArg arg;
    memset(arg.ht, 0, sizeof(arg.ht));
    snprintf(arg.ht, sizeof(arg.ht), "MOVE /x HTTTP/1.0\r\nBadHeaderLineWithNoColonAnywhere\r\n\r\n");

    int sig = bombfuzzer_run_isolated(runParse, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("missing header separator", seed, arg.ht, sig);
}

// class 4: no "\r\n\r\n" anywhere - count_headers()'s headers_end becomes NULL, compared against a
// real pointer. Doesn't crash in practice, kept as a boundary case
static void case_missing_body_separator(unsigned int seed)
{
    ParseArg arg;
    memset(arg.ht, 0, sizeof(arg.ht));
    snprintf(arg.ht, sizeof(arg.ht), "MOVE /x HTTTP/1.0\r\nContent-Length: 4");

    int sig = bombfuzzer_run_isolated(runParse, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("missing body separator", seed, arg.ht, sig);
}

// class 5: msg_add_header() never checks n_headers against MAX_HEADERS - the 17th call writes
// past the fixed-size headers[] array
static void doOverflowHeaders(void* arg)
{
    (void)arg;
    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    req_init(&msg, "MOVE", "/room/0/player/1", "HTTTP/1.0");

    char fields[24][16];
    char values[24][16];
    for (int i = 0; i < 24; i++) {
        snprintf(fields[i], sizeof(fields[i]), "X-Field-%d", i);
        snprintf(values[i], sizeof(values[i]), "value-%d", i);
        msg_add_header(&msg, fields[i], values[i]);
    }
}

static void case_too_many_headers(unsigned int seed)
{
    int sig = bombfuzzer_run_isolated(doOverflowHeaders, NULL, BOMBFUZZER_TIMEOUT_SEC);
    char actual[128];
    snprintf(actual, sizeof(actual), sig > 0 ? "crashed with signal %d writing past headers[MAX_HEADERS]" : "completed without crashing (Valgrind may still show an invalid write)",
             sig);
    bombfuzzer_report(sig == 0, "bombfuzzer_hypertext", "more than MAX_HEADERS headers", seed, NULL, 0,
                      "msg_add_header rejects a header once n_headers reaches MAX_HEADERS", actual);
}

// class 6: convert_to_hypertext's header loop is `for (i = 0; i < n_headers || offset < MAX_BUF; i++)`
// - that `||` should be `&&`. With few headers and a short body it keeps indexing headers[] past
// what was populated, eventually reading out of bounds and dereferencing garbage as field/value
static void doConvertFewHeaders(void* arg)
{
    (void)arg;
    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    req_init(&msg, "MOVE", "/room/0/player/1", "HTTTP/1.0");
    msg_add_header(&msg, "Player-Id", "1");
    msg_add_body(&msg, "x");

    HyperText out;
    convert_to_hypertext(&msg, out);
}

static void case_convert_few_headers(unsigned int seed)
{
    int sig = bombfuzzer_run_isolated(doConvertFewHeaders, NULL, BOMBFUZZER_TIMEOUT_SEC);
    char actual[160];
    snprintf(actual, sizeof(actual),
             sig > 0 ? "crashed with signal %d - convert_to_hypertext read headers[] out of bounds" : "completed without crashing",
             sig);
    bombfuzzer_report(sig == 0, "bombfuzzer_hypertext", "convert_to_hypertext with few headers", seed, NULL, 0,
                      "convert_to_hypertext stops its header loop once i reaches n_headers", actual);
}

// class 7: msg_add_body rejects bodies at/over MAX_BUF
static void case_oversized_body(unsigned int seed)
{
    char* body = malloc(MAX_BUF + 16);
    memset(body, 'A', MAX_BUF + 15);
    body[MAX_BUF + 15] = '\0';

    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    int rc = msg_add_body(&msg, body);

    bombfuzzer_report(rc == -1, "bombfuzzer_hypertext", "oversized body", seed, NULL, (uint64_t)strlen(body),
                      "msg_add_body rejects a body at/over MAX_BUF", "msg_add_body accepted an oversized body");
    free(body);
}

// class 8: msg_add_header must reject '\r'/'\n'/':' in the field name, and '\r'/'\n' (but not ':',
// which values legitimately need - eg. a Date header's "12:30:00") in the value
static void case_illegal_chars(unsigned int seed)
{
    const char* badFields[] = {"Bad\rField", "Bad\nField", "Bad:Field"};
    for (size_t i = 0; i < sizeof(badFields) / sizeof(badFields[0]); i++) {
        ParsedMsgHT msg;
        memset(&msg, 0, sizeof(msg));
        int rc = msg_add_header(&msg, badFields[i], "clean-value");
        bombfuzzer_report(rc == -1, "bombfuzzer_hypertext", "illegal header field", seed,
                          (const unsigned char*)badFields[i], strlen(badFields[i]),
                          "msg_add_header rejects fields containing CR/LF/colon",
                          "msg_add_header accepted a field with an illegal character");
    }

    const char* badValues[] = {"has\rcr", "has\nlf"};
    for (size_t i = 0; i < sizeof(badValues) / sizeof(badValues[0]); i++) {
        ParsedMsgHT msg;
        memset(&msg, 0, sizeof(msg));
        int rc = msg_add_header(&msg, "Content-Type", badValues[i]);
        bombfuzzer_report(rc == -1, "bombfuzzer_hypertext", "illegal header value", seed,
                          (const unsigned char*)badValues[i], strlen(badValues[i]),
                          "msg_add_header rejects values containing CR/LF",
                          "msg_add_header accepted a value with an illegal character");
    }

    ParsedMsgHT msg;
    memset(&msg, 0, sizeof(msg));
    int rc = msg_add_header(&msg, "Date", "12:30:00");
    bombfuzzer_report(rc == 0, "bombfuzzer_hypertext", "colon legal in header value", seed,
                      (const unsigned char*)"12:30:00", 8,
                      "msg_add_header accepts ':' in a value (colons are only illegal in field names)",
                      "msg_add_header rejected a value containing ':'");
}

// class 9: mutation-based - bit-flip a well-formed request and confirm parse_hypertext never
// crashes/hangs on it, whatever the mutation lands on (a broader, less-targeted complement to the
// hand-picked classes above)
static void doParseMutated(void* arg)
{
    ParsedMsgHT result;
    memset(&result, 0, sizeof(result));
    parse_hypertext(((ParseArg*)arg)->ht, &result);
}

static void case_mutated_request(unsigned int seed)
{
    char raw[256];
    snprintf(raw, sizeof(raw), "MOVE /room/0/player/1 HTTTP/1.0\r\nPlayer-Id: 1\r\nContent-Length: 4\r\n\r\ntest");

    ParseArg arg;
    memset(arg.ht, 0, sizeof(arg.ht));
    memcpy(arg.ht, raw, strlen(raw) + 1);

    int mutations = 1 + (int)(bombfuzzer_rand_byte() % 8);
    for (int i = 0; i < mutations; i++) {
        bombfuzzer_flip_random_bit((unsigned char*)arg.ht, strlen(raw));
    }

    int sig = bombfuzzer_run_isolated(doParseMutated, &arg, BOMBFUZZER_TIMEOUT_SEC);
    reportCrashProbe("mutated well-formed request", seed, arg.ht, sig);
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    unsigned int seed = bombfuzzer_init_seed();

    int n = iterations();
    printf("bombfuzzer_hypertext: running %d cases per equivalence class\n", n);

    for (int i = 0; i < n; i++) {
        unsigned int caseSeed = seed + (unsigned int)i;
        case_wellformed_roundtrip(caseSeed);
        case_missing_token_separator(caseSeed);
        case_missing_header_separator(caseSeed);
        case_missing_body_separator(caseSeed);
        case_too_many_headers(caseSeed);
        case_convert_few_headers(caseSeed);
        case_oversized_body(caseSeed);
        case_illegal_chars(caseSeed);
    }

    int m = mutateIterations();
    printf("bombfuzzer_hypertext: running %d mutation-based cases\n", m);
    for (int i = 0; i < m; i++) {
        case_mutated_request(seed + (unsigned int)i);
    }

    printf("bombfuzzer_hypertext: done\n");
    return 0;
}
