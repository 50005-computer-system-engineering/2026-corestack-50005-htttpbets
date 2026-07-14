#include "message.h"

// MESSAGE FUNCTIONS
// same functions as in common.c in PA2, not exposed to library users
int readBytes(int sockfd, unsigned char **returnBuf, uint64_t length)
{
    // allocate space for the return buffer
    if (*returnBuf != NULL)
    {
        free(*returnBuf);
        *returnBuf = NULL;
    }
    *returnBuf = malloc(length);
    if (*returnBuf == NULL)
    {
        perror("readBytes malloc");
        return -1;
    }

    // loop to receive length number of bytes and write to buffer
    uint64_t bytesReceived = 0;
    while (bytesReceived < length)
    {
        uint64_t remaining = length - bytesReceived;
        ssize_t n = recv(sockfd, *returnBuf + bytesReceived, (size_t)remaining, 0);
        if (n <= 0)
        {
            perror("readBytes recv");
            free(*returnBuf);
            *returnBuf = NULL;
            return -1;
        }
        bytesReceived += (uint64_t)n;
    }

    return 0;
}

int sendBytes(int sockfd, const unsigned char *buf, uint64_t length)
{
    // loop to read length number of bytes from buf pointer location
    uint64_t totalSent = 0;
    while (totalSent < length)
    {
        uint64_t remaining = length - totalSent;
        ssize_t m = send(sockfd, buf + totalSent, (size_t)remaining, 0);
        totalSent += (uint64_t)m;
    }
    return 0;
}

// functions to send and receive complete messages
int receiveMessage(int sockfd, Message **returnPtr)
{
    Message *returnMsg = malloc(sizeof(Message));
    if (returnMsg == NULL)
    {
        perror("readBytes malloc");
        goto fail;
    }

    // read Message header bytes
    printf("receiveMessage: allocated memory for Message, listening for header...\n");
    unsigned char *buffer = NULL;
    if (readBytes(sockfd, &buffer, sizeof(uint8_t)) < 0)
    {
        printf("readMessage: failed to read sourceId\n");
        goto fail;
    } 
    returnMsg->sourceId = *buffer;
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, sizeof(uint8_t)) < 0)
    {
        printf("readMessage: failed to read type\n");
        goto fail;
    } 
    returnMsg->type = *buffer;
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        printf("readMessage: failed to read length\n");
        goto fail;
    } 
    uint32_t lenBytes;
    memcpy(&lenBytes, buffer, sizeof(lenBytes));
    returnMsg->length = ntohl(lenBytes);
    free(buffer);
    buffer = NULL;

    // listen for message considering the header
    printf("readMessage: now listening for message with header:\n\tsourceId: %u\n\ttype: %u\n\tlength: %u\n", returnMsg->sourceId, returnMsg->type, returnMsg->length);
    if (readBytes(sockfd, &buffer, returnMsg->length) < 0)
    {
        printf("readMessage: failed to read length\n");
        goto fail;
    }
    returnMsg->content = malloc(returnMsg->length);
    if (returnMsg->content == NULL)
    {
        perror("receiveMessage malloc");
        goto fail;
    }
    memcpy(returnMsg->content, buffer, returnMsg->length);
    free(buffer);
    buffer = NULL;

    // return this ptr
    *returnPtr = returnMsg;

    return 0;

fail:
    free(buffer);
    if (returnMsg != NULL)
    {
        if (returnMsg->content != NULL)
        {
            free(returnMsg->content);
        }
        free(returnMsg);
    }

    return -1;
}

int sendMessage(int sockfd, const Message completeMsg)
{
    printf("sendMessage: sending message with the header:\n\tsourceId: %u\n\ttype: %u\n\tlength: %u\n", completeMsg.sourceId, completeMsg.type, completeMsg.length);
    sendBytes(sockfd, &completeMsg.sourceId, sizeof(uint8_t));
    sendBytes(sockfd, &completeMsg.type, sizeof(uint8_t));
    uint32_t length = htonl(completeMsg.length);
    sendBytes(sockfd, (const unsigned char *)&length, sizeof(uint32_t));
    sendBytes(sockfd, completeMsg.content, completeMsg.length);
    printf("sendMessage: message sending probably worked\n");
    return 0;
}