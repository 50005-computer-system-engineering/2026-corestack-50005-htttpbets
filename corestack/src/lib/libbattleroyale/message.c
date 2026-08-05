#include "message.h"

// private functions
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
        perror("[readBytes()] malloc");
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
            perror("[readBytes()] recv");
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

// public functions
// app layer message functions
int receiveMessageTCP(int sockfd, Message *returnPtr)
{
    if (returnPtr == NULL)
    {
        LOG_E("[receiveMessageTCP()] pointer passed is not valid place in memory");
        goto fail;
    }

    LOG_I("[receiveMessageTCP()] preparing to receive message");

    // read Message header bytes
    LOG_D("[receiveMessageTCP()] listening for message...");
    unsigned char *buffer = NULL;
    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        LOG_E("[receiveMessageTCP()] failed to read source id");
        goto fail;
    } 
    uint32_t sourceBytes;
    memcpy(&sourceBytes, buffer, sizeof(sourceBytes));
    returnPtr->sourceId = ntohl(sourceBytes);
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        LOG_E("[receiveMessageTCP()] failed to read message type");
        goto fail;
    } 
    MessageType typeBytes;
    memcpy(&typeBytes, buffer, sizeof(typeBytes));
    returnPtr->msgType = (MessageType)ntohl(typeBytes);
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, MSG_CONTENT_LENGTH) < 0)
    {
        LOG_E("[receiveMessageTCP()] failed to read message content");
        goto fail;
    }
    snprintf(returnPtr->msgContent, MSG_CONTENT_LENGTH, buffer);
    buffer = NULL;

    LOG_D("[receiveMessageTCP()] received broadcast with following message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", returnPtr->sourceId, returnPtr->msgType, returnPtr->msgContent);
    LOG_I("[receiveMessageTCP()] finsihed receiving message");

    return 0;

    fail:
    free(buffer);

    return -1;
}

int receiveMessageUDP(int sockfd, Message *returnPtr)
{
    LOG_I("[receiveMessageUDP()] preparing to receive broadcast...");

    // receiving message
    int flagsSet = 0;
    unsigned char *messageBytes = malloc(sizeof(Message)); // TODO fix the message size problem
    if (messageBytes == NULL)
    {
        perror("[receiveMessageUDP()] malloc");
        return -1;
    }

    LOG_D("[receiveMessageUDP()] waiting for bytes");

    if (recvfrom(sockfd, messageBytes, sizeof(Message), flagsSet, NULL, NULL) < 0) // TODO specify message length to something reasonable
    {
        perror("[receiveAppMessageUDP()] recvfrom");
        return -1;
    }
    LOG_D("[receiveMessageUDP()] received raw message bytes");

    // fit bytes into message message struct
    returnPtr = calloc(1, sizeof(Message));
    memcpy(returnPtr, messageBytes, sizeof(Message));

    LOG_D("[receiveMessageUDP()] received broadcast with following message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", returnPtr->sourceId, returnPtr->msgType, returnPtr->msgContent);
    LOG_I("[receiveMessageUDP()] broadcast has been received");

    return 0;
}

int sendMessageTCP(int sockfd, const Message completeMsg)
{
    LOG_I("[sendMessageTCP()] sending message:\n\tsourceId: %u\n\ttype (integerified): %d\n\tcontent: %s", completeMsg.sourceId, completeMsg.msgType, completeMsg.msgContent);
    uint32_t sourceId = htonl(completeMsg.sourceId);
    sendBytes(sockfd, (const unsigned char *)&sourceId, sizeof(uint32_t));
    MessageType type = htonl(completeMsg.msgType);
    sendBytes(sockfd, (const unsigned char *)&type, sizeof(uint32_t));
    sendBytes(sockfd, completeMsg.msgContent, MSG_CONTENT_LENGTH);
    LOG_I("[sendMessageTCP()] pushed all bytes though socket");
    return 0;
}

int sendBroadcastUDP(int sockfd, const Message completeMsg)
{
    LOG_I("[sendMessageUDP()] sending message:\n\tsourceId: %u\n\ttype (integerified): %d\n\tcontent: %s", completeMsg.sourceId, completeMsg.msgType, completeMsg.msgContent);
    
    // preparing parameters for sendto()
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_BROAD),
        .sin_addr.s_addr = INADDR_BROADCAST // broadcast address
    };
    int flagsSet = 0;
    socklen_t addrLen = sizeof(addr);

    // send the entire message over broadcast port
    if (sendto(sockfd, (unsigned char *)&completeMsg, sizeof(Message), flagsSet, (struct sockaddr *)&addr, addrLen) < 0)
    {
        perror("[sendMessageUDP()] sendto");
        return -1;
    }

    // cleanup

    LOG_I("[sendMessageUDP()] broadcast has been sent");
    return 0;
}

// TODO unicasting with UDP