#include "bombfuzzer_common.h"
#include "message.h"
#include <sys/socket.h>

// fuzzes read_bytes/send_bytes (libbattleroyale/message.c) over a UNIX socketpair, which exercises
// the same recv()/send() code paths as a real TCP socket.
//
// Cases:
//   1. well-formed transfer
//   2. length == 0
//   3. peer sends fewer bytes than claimed, then closes
//   4. send_bytes() into a closed read end

#define ITER_DEFAULT 500
#define MAX_PAYLOAD 4096

static int iterations(void)
{
    const char *env = getenv("BOMBFUZZER_ITERS");
    return env != NULL ? atoi(env) : ITER_DEFAULT;
}

static unsigned char *randomPayload(uint64_t length)
{
    unsigned char *buf = malloc(length);
    for (uint64_t i = 0; i < length; i++)
    {
        buf[i] = bombfuzzer_rand_byte();
    }
    return buf;
}

// class 1: well-formed transfer - read_bytes must return the exact bytes send_bytes wrote
static void case_wellformed(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_bytes socketpair");
        return;
    }

    uint64_t length = 1 + (bombfuzzer_rand_u32() % MAX_PAYLOAD);
    unsigned char *payload = randomPayload(length);
    send_bytes(fds[1], payload, length);

    unsigned char *out = NULL;
    int rc = -1;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = read_bytes(fds[0], &out, length);
        bombfuzzer_disarm_timeout();

        int matched = (rc == 0) && (out != NULL) && (memcmp(out, payload, length) == 0);
        bombfuzzer_report(matched, "bombfuzzer_bytes", "well-formed transfer", seed, payload, length,
                           "read_bytes returns 0 with the exact bytes sent",
                           "read_bytes returned an error or mismatched content on a well-formed transfer");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_bytes", "well-formed transfer", seed, payload, length,
                           "read_bytes returns promptly", "read_bytes hung past the timeout");
    }

    free(out);
    free(payload);
    close(fds[0]);
    close(fds[1]);
}

// class 2: length == 0 - malloc(0) is implementation-defined, read_bytes must not treat it as failure
static void case_zero_length(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_bytes socketpair");
        return;
    }

    unsigned char *out = NULL;
    int rc = -1;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = read_bytes(fds[0], &out, 0);
        bombfuzzer_disarm_timeout();

        bombfuzzer_report(rc == 0, "bombfuzzer_bytes", "zero-length read", seed, NULL, 0,
                           "read_bytes returns 0 for a zero-length request",
                           "read_bytes returned an error for length == 0");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_bytes", "zero-length read", seed, NULL, 0,
                           "read_bytes returns promptly", "read_bytes hung past the timeout on length == 0");
    }

    free(out);
    close(fds[0]);
    close(fds[1]);
}

// class 3: peer writes fewer bytes than claimed, then disconnects mid-transfer
static void case_truncated(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_bytes socketpair");
        return;
    }

    uint64_t claimedLength = 256 + (bombfuzzer_rand_u32() % MAX_PAYLOAD);
    uint64_t actualLength = bombfuzzer_rand_u32() % claimedLength;  // strictly less than claimed
    unsigned char *payload = randomPayload(actualLength);
    send_bytes(fds[1], payload, actualLength);
    close(fds[1]);   // disconnect before the claimed length is satisfied

    unsigned char *out = NULL;
    int rc = -2;   // sentinel distinct from read_bytes' own -1/0
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = read_bytes(fds[0], &out, claimedLength);
        bombfuzzer_disarm_timeout();

        bombfuzzer_report(rc == -1 && out == NULL, "bombfuzzer_bytes", "truncated transfer", seed, payload,
                           actualLength,
                           "read_bytes returns -1 and frees its buffer once the peer closes early",
                           "read_bytes did not cleanly report the truncated/closed connection");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_bytes", "truncated transfer", seed, payload, actualLength,
                           "read_bytes returns promptly once the peer closes",
                           "read_bytes hung past the timeout instead of observing EOF");
    }

    free(out);
    free(payload);
    close(fds[0]);
}

// class 4: send_bytes() into an already-closed read end - does a failed send() get reported?
static void case_send_after_peer_close(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_bytes socketpair");
        return;
    }
    close(fds[0]);   // "peer" hangs up its read end first

    uint64_t length = 1 + (bombfuzzer_rand_u32() % MAX_PAYLOAD);
    unsigned char *payload = randomPayload(length);

    int rc = -2;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = send_bytes(fds[1], payload, length);
        bombfuzzer_disarm_timeout();

        bombfuzzer_report(rc < 0, "bombfuzzer_bytes", "send after peer close", seed, payload, length,
                           "send_bytes returns a negative code once send() fails on a closed peer",
                           "send_bytes reported success (returned 0) despite send() failing");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_bytes", "send after peer close", seed, payload, length,
                           "send_bytes returns promptly", "send_bytes hung past the timeout");
    }

    free(payload);
    close(fds[1]);
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);   // a broken pipe should surface as send()'s return value, not kill us
    unsigned int seed = bombfuzzer_init_seed();

    int n = iterations();
    printf("bombfuzzer_bytes: running %d cases per equivalence class\n", n);

    for (int i = 0; i < n; i++)
    {
        case_wellformed(seed + (unsigned int)i);
        case_zero_length(seed + (unsigned int)i);
        case_truncated(seed + (unsigned int)i);
        case_send_after_peer_close(seed + (unsigned int)i);
    }

    printf("bombfuzzer_bytes: done, %d cases per class\n", n);
    return 0;
}
