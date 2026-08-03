#include "message.h"
#include "common.h"

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
        if (m < 0)
        {
            perror("[sendBytes()] send");
            return -1;
        }
        if (m == 0)
        {
            LOG_E("[sendBytes()] send() returned 0, aborting to avoid infinite loop");
            return -1;
        }
        totalSent += (uint64_t)m;
    }
    return 0;
}

// public functions
// registration functions
int registerNewClient(Endpoint *newClient)
{
    if (newClient == NULL) 
    {
        LOG_E("[registerNewClient()] invalid newClient ptr passed");
    }

    LOG_I("[registerNewClient()] registering client with id %u\n", newClient->id);

    unsigned char *buffer = NULL;

    // // step 1: receive client nonce
    // buffer = malloc(NONCE_LEN);
    // if (buffer == NULL)
    // {
    //     perror("[registerNewClient()] malloc");
    //     return -1;
    // }
    // if (readBytes(newClient->socks->tcp, &buffer, NONCE_LEN) < 0)
    // {
    //     LOG_E("[registerNewClient()] failed to read NONCE");
    //     return -1;
    // }
    // memcpy(newClient->token, buffer, NONCE_LEN);
    // free(buffer);
    // buffer = NULL;
    // TODO: implement authentication with nonce signing
    
    // step 2: send user ID
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL)
    {
        perror("[registerNewClient()] malloc");
        return -1;
    }
    uint32_t idBytes = htonl(newClient->id);
    if (sendBytes(newClient->socks->tcp, (unsigned char *)&idBytes, sizeof(uint32_t)) < 0)
    {
        LOG_E("[registerNewClient()] failed to send source ID");
        return -1;
    }

    LOG_I("[registerNewClient()] registration complete");

    return 0;
}

// client function which waits for the client 
int registerWithServer(Endpoint *myClient)
{
    // // step 1: send nonce
    unsigned char *buffer = NULL;
    // buffer = malloc(NONCE_LEN);
    // // TODO nonce generation
    // // buffer = generateNonce();
    // if (buffer == NULL)
    // {
    //     perror("[registerWithServer()] malloc");
    //     return -1;
    // }

    // if (sendBytes(myClient->socks->tcp, buffer, NONCE_LEN) < 0)
    // {
    //     LOG_E("[registerWithServer()] failed to send NONCE");
    //     return -1;
    // }
    // free(buffer);
    // buffer = NULL;
    // // TODO implement authentication by verification of signed nonce

    // step 2: receive user id and save to Endpoint
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL)
    {
        perror("[registerWithServer()] malloc");
        return -1;
    }
    if (readBytes(myClient->socks->tcp, &buffer, sizeof(uint32_t)) < 0)
    {
        LOG_E("[registerWithServer()] failed to read sourceId");
        return -1;
    } 
    uint32_t sourceBytes;
    memcpy(&sourceBytes, buffer, sizeof(sourceBytes));
    myClient->id = ntohl(sourceBytes);
    free(buffer);
    buffer = NULL;

    return 0;
}

// TODO administrative messages (kick/ban)
// admin functions

// app layer message functions
int receiveMessage(int sockfd, Message **returnPtr)
{
    Message *returnMsg = calloc(1, sizeof(Message));

    LOG_I("[receiveMessage()] preparing to receive message");

    if (returnMsg == NULL)
    {
        perror("[receiveMessage()] malloc");
        goto fail;
    }

    // read Message header bytes
    LOG_D("[receiveMessage()] allocated memory for Message, listening for header...");
    unsigned char *buffer = NULL;
    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        LOG_E("[receiveMessage()] failed to read sourceId\n");
        goto fail;
    } 
    uint32_t sourceBytes;
    memcpy(&sourceBytes, buffer, sizeof(sourceBytes));
    returnMsg->sourceId = ntohl(sourceBytes);
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        LOG_E("[receiveMessage()] failed to read length");
        goto fail;
    } 
    uint32_t lenBytes;
    memcpy(&lenBytes, buffer, sizeof(lenBytes));
    returnMsg->length = ntohl(lenBytes);
    free(buffer);
    buffer = NULL;

    // listen for message considering the header
    LOG_D("[receiveMessage()] now listening for message with header:\n\tsourceId: %u\n\tlength: %u\n", returnMsg->sourceId, returnMsg->length);
    if (readBytes(sockfd, &buffer, returnMsg->length) < 0)
    {
        LOG_E("[receiveMessage()] failed to read message");
        goto fail;
    }
    returnMsg->content = malloc(returnMsg->length + 1);
    if (returnMsg->content == NULL)
    {
        perror("[receiveMessage()] malloc");
        free(buffer);
        goto fail;
    }
    memcpy(returnMsg->content, buffer, returnMsg->length);
    returnMsg->content[returnMsg->length] = '\0';
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
    LOG_I("[sendMessage()] sending message with the header:\n\tsourceId: %u\n\tlength: %u", completeMsg.sourceId, completeMsg.length);
    uint32_t sourceId = htonl(completeMsg.sourceId);
    sendBytes(sockfd, (const unsigned char *)&sourceId, sizeof(uint32_t));
    uint32_t length = htonl(completeMsg.length);
    sendBytes(sockfd, (const unsigned char *)&length, sizeof(uint32_t));
    sendBytes(sockfd, completeMsg.content, completeMsg.length);
    LOG_I("[sendMessage()] pushed all bytes though socket");
    return 0;
}

int receiveBroadcast(int sockfd, Message **returnPtr)
{
    LOG_I("[receiveBroadcast()] preparing to receive broadcast...");

    // receiving message
    int flagsSet = 0;
    unsigned char *messageBytes = malloc(4096); // TODO fix the message size problem
    if (messageBytes == NULL)
    {
        perror("[receiveBroadcast()] malloc");
        return -1;
    }

    LOG_D("[receiveBroadcast()] waiting for bytes");

    if (recvfrom(sockfd, messageBytes, 4096, flagsSet, NULL, NULL) < 0) // TODO specify message length to something reasonable
    {
        perror("[receiveBroadcast()] recvfrom");
        return -1;
    }
    LOG_D("[receiveBroadcast()] received raw message bytes");

    // fit bytes into message message struct
    Message *returnMsg = calloc(1, sizeof(Message));
    returnMsg->sourceId = *messageBytes;
    returnMsg->length = *(messageBytes+sizeof(returnMsg->length));
    returnMsg->content = malloc(returnMsg->length);
    memcpy(returnMsg->content, messageBytes+sizeof(returnMsg->sourceId)+sizeof(returnMsg->length), returnMsg->length);

    LOG_D("[receiveBroadcast()] received broadcast with following message:\n\tsource: %u\n\tlength: %u", returnMsg->sourceId, returnMsg->length);

    LOG_I("[receiveBroadcast()] broadcast has been received");
    *returnPtr = returnMsg;

    return 0;
}

int sendBroadcast(int sockfd, const Message completeMsg)
{
    LOG_I("[sendBroadcast()] sending message with the header:\n\tsourceId: %u\n\tlength: %u", completeMsg.sourceId, completeMsg.length);
    
    // preparing parameters for sendto()
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_BROAD),
        .sin_addr.s_addr = INADDR_BROADCAST // broadcast address
    };
    int flagsSet = 0;
    socklen_t addrLen = sizeof(addr);
    
    // prepare message as raw bytes
    size_t msgLen = sizeof(completeMsg.sourceId) + sizeof(completeMsg.length) + completeMsg.length;
    unsigned char *messageBytes = malloc(msgLen);
    if (messageBytes == NULL)
    {
        perror("[sendBroadcast()] malloc");
        return -1;
    }
    memcpy(messageBytes, &completeMsg.sourceId, sizeof(completeMsg.sourceId));
    memcpy(messageBytes + sizeof(completeMsg.sourceId), &completeMsg.length, sizeof(completeMsg.length));
    memcpy(messageBytes + sizeof(completeMsg.sourceId) + sizeof(completeMsg.length), completeMsg.content, completeMsg.length);
    LOG_D("[sendBroadcast()] copied Message into raw bytes");

    // send the entire message over broadcast port
    if (sendto(sockfd, messageBytes, msgLen, flagsSet, (struct sockaddr *)&addr, addrLen) < 0)
    {
        perror("[sendBroadcast()] sendto");
        return -1;
    }

    // cleanup
    free(messageBytes);
    messageBytes = NULL;

    LOG_I("[sendBroadcast()] broadcast has been sent");
    return 0;
}