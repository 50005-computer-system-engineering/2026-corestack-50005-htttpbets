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

// public functions
// registration functions
int registerNewClient(Endpoint *newClient)
{
    if (newClient == NULL) 
    {
        printf("libhtttp/registration.h registerNewClient: invalid newClient ptr passed\n");
    }

    printf("libhtttp/registration.h registerNewClient: registering client with id %u\n", newClient->id);

    unsigned char *buffer = NULL;

    // step 1: receive client nonce
    buffer = malloc(NONCE_LEN);
    if (buffer == NULL)
    {
        perror("libhtttp/registration.h malloc");
        return -1;
    }
    if (readBytes(newClient->socks->tcp, &buffer, NONCE_LEN) < 0)
    {
        printf("libhtttp/registration.h registerNewClient: failed to read NONCE\n");
        return -1;
    }
    memcpy(newClient->token, buffer, NONCE_LEN);
    free(buffer);
    buffer = NULL;
    // TODO: implement authentication with nonce signing
    
    // step 2: send user ID
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL)
    {
        perror("libhtttp/registration.h malloc");
        return -1;
    }
    uint32_t idBytes = htonl(newClient->id);
    if (sendBytes(newClient->socks->tcp, (unsigned char *)&idBytes, sizeof(uint32_t)) < 0)
    {
        printf("libhtttp/registration.h registerNewClient: failed to send source ID\n");
        return -1;
    }

    printf("libhtttp/registration.h registerNewClient: registration complete\n");

    return 0;
}

// client function which waits for the client 
int registerWithServer(Endpoint *myClient)
{
    // step 1: send nonce
    unsigned char *buffer = NULL;
    buffer = malloc(NONCE_LEN);
    // TODO nonce generation
    // buffer = generateNonce();
    if (buffer == NULL)
    {
        perror("libhtttp/registration.h malloc");
        return -1;
    }

    if (sendBytes(myClient->socks->tcp, buffer, NONCE_LEN) < 0)
    {
        printf("libhtttp/registration.h registerNewClient: failed to send NONCE\n");
        return -1;
    }
    free(buffer);
    buffer = NULL;
    // TODO implement authentication by verification of signed nonce

    // step 2: receive user id and save to Endpoint
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL)
    {
        perror("libhtttp/registration.h malloc");
        return -1;
    }
    if (readBytes(myClient->socks->tcp, &buffer, sizeof(uint32_t)) < 0)
    {
        printf("readMessage: failed to read sourceId\n");
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
    Message *returnMsg = malloc(sizeof(Message));
    if (returnMsg == NULL)
    {
        perror("readBytes malloc");
        goto fail;
    }

    // read Message header bytes
    printf("receiveMessage: allocated memory for Message, listening for header...\n");
    unsigned char *buffer = NULL;
    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        printf("readMessage: failed to read sourceId\n");
        goto fail;
    } 
    uint32_t sourceBytes;
    memcpy(&sourceBytes, buffer, sizeof(sourceBytes));
    returnMsg->sourceId = ntohl(sourceBytes);
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
    printf("receiveMessage: now listening for message with header:\n\tsourceId: %u\n\tlength: %u\n", returnMsg->sourceId, returnMsg->length);
    if (readBytes(sockfd, &buffer, returnMsg->length) < 0)
    {
        printf("readMessage: failed to read message\n");
        goto fail;
    }
    // returnMsg->content = malloc(returnMsg->length);
    // if (returnMsg->content == NULL)
    // {
    //     perror("receiveMessage malloc");
    //     goto fail;
    // }
    // memcpy(returnMsg->content, buffer, returnMsg->length);
    // free(buffer);
    returnMsg->content = buffer;
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
    printf("sendMessage: sending message with the header:\n\tsourceId: %u\n\tlength: %u\n", completeMsg.sourceId, completeMsg.length);
    uint32_t sourceId = htonl(completeMsg.sourceId);
    sendBytes(sockfd, (const unsigned char *)&sourceId, sizeof(uint32_t));
    uint32_t length = htonl(completeMsg.length);
    sendBytes(sockfd, (const unsigned char *)&length, sizeof(uint32_t));
    sendBytes(sockfd, completeMsg.content, completeMsg.length);
    printf("sendMessage: message sending probably worked\n");
    return 0;
}
