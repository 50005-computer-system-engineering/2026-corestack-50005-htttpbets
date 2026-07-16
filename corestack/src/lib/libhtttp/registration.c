#include "registration.h"

#include "connection.h"
#include "message.h"

int registerNewClient(Server *myServer, int clientId)
{
    // check if server exists
    if (myServer == NULL)
    {
        printf("libhtttp/something.h sendClientDetails: server does not exist\n");
        return -1;
    }

    Record *targetClient = &myServer->clientList[clientId - 1];

    unsigned char *buffer = NULL;

    // step 1: receive client nonce
    buffer = malloc(NONCE_LEN);
    if (buffer == NULL)
    {
        perror("libhtttp/something.h malloc");
        return -1;
    }
    if (readBytes(targetClient.socks->tcp, &buffer, NONCE_LEN) < 0)
    {
        printf("libhtttp/something.h registerNewClient: failed to read NONCE");
        return -1;
    }
    targetClient->token = buffer;
    free(buffer);
    buffer = NULL;
    // TODO: implement authentication with nonce message signing
    
    // step 2: send user ID
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL)
    {
        perror("libhtttp/something.h malloc");
        return -1;
    }
    uint32_t idBytes = htonl(clientId);
    if (sendBytes(targetClient.socks->tcp, &idBytes, sizeof(uint32_t)) < 0)
    {
        printf("libhtttp/something.h registerNewClient: failed to send source ID");
        return -1;
    }

    return 0;
}

// client function which waits for the client 
int registerWithServer(Record *myClient)
{
    // step 1: send nonce
    unsigned char *buffer = NULL;
    buffer = malloc(NONCE_LEN);
    // TODO nonce generation
    // buffer = generateNonce();
    if (buffer == NULL)
    {
        perror("libhtttp/something.h malloc");
        return -1;
    }

    if (sendBytes(myClient->socks->tcp, buffer, NONCE_LEN) < 0)
    {
        printf("libhtttp/something.h registerNewClient: failed to send NONCE");
        return -1;
    }
    free(buffer);
    buffer = NULL;
    // TODO implement authentication by verification of signed nonce

    // step 2: receive user id and save to record
    buffer = malloc(uint32_t);
    if (buffer == NULL)
    {
        perror("libhtttp/something.h malloc");
        return -1;
    }
    if (readBytes(targetClient.socks->tcp, &buffer, NONCE_LEN) < 0)
    {
        printf("libhtttp/something.h registerNewClient: failed to read NONCE");
        return -1;
    }
    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        printf("readMessage: failed to read sourceId\n");
        goto fail;
    } 
    uint32_t sourceBytes;
    memcpy(&sourceBytes, buffer, sizeof(sourceBytes));
    myClient->id = ntohl(sourceBytes);
    free(buffer);
    buffer = NULL;

    return 0;
}