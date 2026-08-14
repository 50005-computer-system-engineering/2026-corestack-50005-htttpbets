#include "bombfuzzer_common.h"
#include "message.h"
#include <sys/socket.h>

// fuzzes receive_message_tcp/send_message_tcp, the app-layer framing on top of read_bytes/
// send_bytes. Still a socketpair (see bombfuzzer_bytes.c) so we can write raw wire bytes directly,
// bypassing send_message_tcp() - what an attacker would do.
//
// Wire format (big-endian): [ source_id: 4 bytes ][ msg_type: 4 bytes ][ msg_content: 1024 bytes ]
//
// Cases:
//   1. well-formed round trip
//   2. header itself truncated (peer disconnects mid source_id/msg_type)
//   3. content truncated (full header, but fewer than 1024 content bytes, then disconnect)
//   4. msg_type outside the declared MessageType range (still well-formed framing-wise)

#define ITER_DEFAULT 200

static int iterations(void)
{
    const char *env = getenv("BOMBFUZZER_ITERS");
    return env != NULL ? atoi(env) : ITER_DEFAULT;
}

static void randomContent(unsigned char content[MSG_CONTENT_LENGTH])
{
    for (int i = 0; i < MSG_CONTENT_LENGTH; i++)
    {
        content[i] = bombfuzzer_rand_byte();
    }
}

static void writeRawHeader(int fd, uint32_t sourceId, uint32_t msgType)
{
    uint32_t netSourceId = htonl(sourceId);
    uint32_t netType = htonl(msgType);
    write(fd, &netSourceId, sizeof(netSourceId));
    write(fd, &netType, sizeof(netType));
}

// class 1: well-formed round trip via the real public send_message_tcp()/receive_message_tcp() pair
static void case_wellformed(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_message socketpair");
        return;
    }

    Message outgoing = {.source_id = bombfuzzer_rand_u32(), .msg_type = MSG_APP};
    randomContent(outgoing.msg_content);
    send_message_tcp(fds[1], outgoing);

    Message incoming;
    memset(&incoming, 0, sizeof(incoming));
    int rc = -2;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = receive_message_tcp(fds[0], &incoming);
        bombfuzzer_disarm_timeout();

        int matched = (rc == 0) && (incoming.source_id == outgoing.source_id) && (incoming.msg_type == outgoing.msg_type) &&
                      (memcmp(incoming.msg_content, outgoing.msg_content, MSG_CONTENT_LENGTH) == 0);
        bombfuzzer_report(matched, "bombfuzzer_message", "well-formed round trip", seed, outgoing.msg_content,
                           MSG_CONTENT_LENGTH, "receive_message_tcp reproduces the source_id/msg_type/content sent",
                           "receive_message_tcp returned an error or mismatched a well-formed message");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_message", "well-formed round trip", seed, outgoing.msg_content, MSG_CONTENT_LENGTH,
                           "receive_message_tcp returns promptly", "receive_message_tcp hung past the timeout");
    }

    close(fds[0]);
    close(fds[1]);
}

// class 2: the header itself is truncated (peer disconnects before all 8 header bytes arrive)
static void case_truncated_header(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_message socketpair");
        return;
    }

    uint8_t partialLen = bombfuzzer_rand_byte() % 8;   // 0-7 bytes, always short of the 8-byte header
    unsigned char partial[8];
    for (uint8_t i = 0; i < partialLen; i++)
    {
        partial[i] = bombfuzzer_rand_byte();
    }
    write(fds[1], partial, partialLen);
    close(fds[1]);

    Message incoming;
    memset(&incoming, 0, sizeof(incoming));
    int rc = -2;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = receive_message_tcp(fds[0], &incoming);
        bombfuzzer_disarm_timeout();

        bombfuzzer_report(rc == -1, "bombfuzzer_message", "truncated header", seed, partial, partialLen,
                           "receive_message_tcp returns -1 when the header itself is incomplete",
                           "receive_message_tcp did not cleanly report an incomplete header");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_message", "truncated header", seed, partial, partialLen,
                           "receive_message_tcp returns promptly", "receive_message_tcp hung past the timeout");
    }

    close(fds[0]);
}

// class 3: full header, but the peer disconnects partway through the 1024-byte content block
static void case_truncated_content(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_message socketpair");
        return;
    }

    writeRawHeader(fds[1], bombfuzzer_rand_u32(), MSG_APP);
    uint32_t actualLen = bombfuzzer_rand_u32() % MSG_CONTENT_LENGTH;   // strictly less than MSG_CONTENT_LENGTH
    unsigned char *partialContent = malloc(actualLen);
    for (uint32_t i = 0; i < actualLen; i++)
    {
        partialContent[i] = bombfuzzer_rand_byte();
    }
    write(fds[1], partialContent, actualLen);
    close(fds[1]);

    Message incoming;
    memset(&incoming, 0, sizeof(incoming));
    int rc = -2;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = receive_message_tcp(fds[0], &incoming);
        bombfuzzer_disarm_timeout();

        bombfuzzer_report(rc == -1, "bombfuzzer_message", "truncated content", seed, partialContent, actualLen,
                           "receive_message_tcp returns -1 once the peer disconnects mid-content",
                           "receive_message_tcp did not cleanly report the short/closed connection");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_message", "truncated content", seed, partialContent, actualLen,
                           "receive_message_tcp returns promptly once the peer closes",
                           "receive_message_tcp hung past the timeout");
    }

    free(partialContent);
    close(fds[0]);
}

// class 4: msg_type outside the declared MessageType enum range - framing is otherwise well-formed
static void case_invalid_type(unsigned int seed)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        perror("bombfuzzer_message socketpair");
        return;
    }

    uint32_t wildType = bombfuzzer_rand_u32();   // almost certainly not a declared MessageType value
    unsigned char content[MSG_CONTENT_LENGTH];
    randomContent(content);
    writeRawHeader(fds[1], bombfuzzer_rand_u32(), wildType);
    write(fds[1], content, MSG_CONTENT_LENGTH);
    close(fds[1]);

    Message incoming;
    memset(&incoming, 0, sizeof(incoming));
    int rc = -2;
    if (sigsetjmp(bombfuzzer_jmpbuf, 1) == 0)
    {
        bombfuzzer_arm_timeout();
        rc = receive_message_tcp(fds[0], &incoming);
        bombfuzzer_disarm_timeout();

        bombfuzzer_report(rc == 0, "bombfuzzer_message", "out-of-range msg_type", seed, content, MSG_CONTENT_LENGTH,
                           "receive_message_tcp accepts any msg_type value without crashing",
                           "receive_message_tcp crashed or errored on an out-of-range msg_type");
    }
    else
    {
        bombfuzzer_report(0, "bombfuzzer_message", "out-of-range msg_type", seed, content, MSG_CONTENT_LENGTH,
                           "receive_message_tcp returns promptly", "receive_message_tcp hung past the timeout");
    }

    close(fds[0]);
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    unsigned int seed = bombfuzzer_init_seed();

    int n = iterations();
    printf("bombfuzzer_message: running %d cases per equivalence class\n", n);

    for (int i = 0; i < n; i++)
    {
        unsigned int caseSeed = seed + (unsigned int)i;
        case_wellformed(caseSeed);
        case_truncated_header(caseSeed);
        case_truncated_content(caseSeed);
        case_invalid_type(caseSeed);
    }

    printf("bombfuzzer_message: done\n");
    return 0;
}
