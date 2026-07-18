#include "common.h"
#include "message.h"
#include "registration.h"

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