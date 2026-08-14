#include "bombfuzzer_common.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <errno.h>

sigjmp_buf bombfuzzer_jmpbuf;

// only siglongjmp out if a case is actually armed - guards against a stray/late SIGALRM
static volatile sig_atomic_t timeoutArmed = 0;
static int findingCount = 0;

// tracks distinct (harness, description) pairs seen so far, so a bug hit on every case logs one
// finding instead of N near-identical ones (occurrence counts still print at exit, see below)
#define MAX_TRACKED_CLASSES 128
static struct
{
    char key[160];
    int count;
} trackedClasses[MAX_TRACKED_CLASSES];
static int trackedClassCount = 0;

// private functions
static void alarmHandler(int sig)
{
    (void)sig;
    if (timeoutArmed)
    {
        siglongjmp(bombfuzzer_jmpbuf, 1);
    }
}

static void ensureFindingsDir(void)
{
    if (mkdir(BOMBFUZZER_FINDINGS_DIR, 0755) < 0 && errno != EEXIST)
    {
        perror("bombfuzzer: mkdir findings dir");
    }
}

// returns the running occurrence count for this (harness, description) pair, registering it as a
// newly-seen class the first time it's asked about
static int noteOccurrence(const char *harness, const char *description)
{
    char key[160];
    snprintf(key, sizeof(key), "%s | %s", harness, description);

    for (int i = 0; i < trackedClassCount; i++)
    {
        if (strcmp(trackedClasses[i].key, key) == 0)
        {
            trackedClasses[i].count++;
            return trackedClasses[i].count;
        }
    }
    if (trackedClassCount < MAX_TRACKED_CLASSES)
    {
        snprintf(trackedClasses[trackedClassCount].key, sizeof(trackedClasses[trackedClassCount].key), "%s", key);
        trackedClasses[trackedClassCount].count = 1;
        trackedClassCount++;
    }
    return 1;
}

static void printSummaryOnExit(void)
{
    if (trackedClassCount == 0)
    {
        return;
    }
    printf("bombfuzzer: %d distinct finding class(es) this run:\n", trackedClassCount);
    for (int i = 0; i < trackedClassCount; i++)
    {
        printf("  - %s (x%d)\n", trackedClasses[i].key, trackedClasses[i].count);
    }
}

// caps this process' virtual address space (inherited across fork() by every forked role), so a
// runaway allocation - intentional (a UINT32_MAX length claim) or bug-triggered - fails cleanly
// instead of eating host RAM. 3GB rather than 1GB: Valgrind needs real headroom of its own too.
#define BOMBFUZZER_MEM_LIMIT_BYTES (3UL << 30)

static void capMemoryUsage(void)
{
    struct rlimit lim = {.rlim_cur = BOMBFUZZER_MEM_LIMIT_BYTES, .rlim_max = BOMBFUZZER_MEM_LIMIT_BYTES};
    if (setrlimit(RLIMIT_AS, &lim) < 0)
    {
        perror("bombfuzzer: setrlimit(RLIMIT_AS)");
    }
}

// public functions
unsigned int bombfuzzer_init_seed(void)
{
    capMemoryUsage();
    ensureFindingsDir();

    unsigned int seed;
    const char *envSeed = getenv("BOMBFUZZER_SEED");
    if (envSeed != NULL)
    {
        seed = (unsigned int)strtoul(envSeed, NULL, 10);
    }
    else
    {
        seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    }
    srand(seed);
    atexit(printSummaryOnExit); // skipped by forked roles that _exit() instead of returning
    printf("bombfuzzer: seed = %u (rerun with BOMBFUZZER_SEED=%u to reproduce this run)\n", seed, seed);
    return seed;
}

uint8_t bombfuzzer_rand_byte(void)
{
    return (uint8_t)(rand() & 0xFF);
}

uint32_t bombfuzzer_rand_u32(void)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
    {
        v = (v << 8) | bombfuzzer_rand_byte();
    }
    return v;
}

uint64_t bombfuzzer_rand_u64(void)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
    {
        v = (v << 8) | bombfuzzer_rand_byte();
    }
    return v;
}

void bombfuzzer_flip_random_bit(unsigned char *buf, uint64_t len)
{
    if (len == 0)
    {
        return;
    }
    uint64_t byteIdx = bombfuzzer_rand_u64() % len;
    uint8_t bitIdx = bombfuzzer_rand_byte() % 8;
    buf[byteIdx] ^= (unsigned char)(1 << bitIdx);
}

void bombfuzzer_arm_timeout_secs(unsigned int secs)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarmHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // deliberately no SA_RESTART - a blocked recv()/connect() must return EINTR
    sigaction(SIGALRM, &sa, NULL);
    timeoutArmed = 1;
    alarm(secs);
}

void bombfuzzer_arm_timeout(void)
{
    bombfuzzer_arm_timeout_secs(BOMBFUZZER_TIMEOUT_SEC);
}

void bombfuzzer_disarm_timeout(void)
{
    alarm(0);
    timeoutArmed = 0;
}

void bombfuzzer_log_finding(const BombfuzzerFinding *finding)
{
    if (noteOccurrence(finding->harness, finding->description) > 1)
    {
        return; // already logged this class once this run - see noteOccurrence's comment above
    }

    findingCount++;
    // pid in the filename so forked roles don't collide on the same findingCount
    char path[256];
    snprintf(path, sizeof(path), "%s/%s_%d_%03d.log", BOMBFUZZER_FINDINGS_DIR, finding->harness,
             (int)getpid(), findingCount);

    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        perror("bombfuzzer_log_finding fopen");
        return;
    }

    fprintf(f, "id: %s_%d_%03d\n", finding->harness, (int)getpid(), findingCount);
    fprintf(f, "harness: %s\n", finding->harness);
    fprintf(f, "description: %s\n", finding->description);
    fprintf(f, "seed: %u\n", finding->seed);
    fprintf(f, "precondition: fresh call, no prior state carried over from another case\n");
    fprintf(f, "expected: %s\n", finding->expected);
    fprintf(f, "actual: %s\n", finding->actual);

    if (finding->input != NULL)
    {
        fprintf(f, "input (%lu bytes, hex): ", (unsigned long)finding->input_len);
        for (uint64_t i = 0; i < finding->input_len; i++)
        {
            fprintf(f, "%02x", finding->input[i]);
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("bombfuzzer: FINDING logged -> %s (%s)\n", path, finding->description);
}

void bombfuzzer_report(int ok, const char *harness, const char *description, unsigned int seed,
                       const unsigned char *input, uint64_t input_len,
                       const char *expected, const char *actual)
{
    if (ok)
    {
        return;
    }
    BombfuzzerFinding finding = {
        .harness = harness,
        .description = description,
        .seed = seed,
        .input = input,
        .input_len = input_len,
        .expected = expected,
        .actual = actual,
    };
    bombfuzzer_log_finding(&finding);
}

int bombfuzzer_run_isolated(BombfuzzerCaseFn fn, void *arg, unsigned int timeoutSecs)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("bombfuzzer_run_isolated fork");
        return -1;
    }
    if (pid == 0)
    {
        fn(arg);
        _exit(0);
    }

    time_t deadline = time(NULL) + (time_t)timeoutSecs;
    int status = 0;
    for (;;)
    {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid)
        {
            break;
        }
        if (time(NULL) >= deadline)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return -1;
        }
        usleep(1000);
    }
    return WIFSIGNALED(status) ? WTERMSIG(status) : 0;
}
