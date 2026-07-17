#include "lib/libhtttp/client.h"
#include "connection.h"
#include "registration.h"

int createClient(LibhtttpClient **clientPtr)
{
    Record *newClient = malloc(sizeof(Record));
    if (newClient == NULL)
    {
        perror("client malloc");
        return -1;
    }
    printf("libhtttp/client createClient: new client created\n");
    *clientPtr = newClient;
    return 0;
}

int joinLobby(LibhtttpClient *clientPtr, char *ipAddress)
{
    Record *thisClient = clientPtr;
    if (createSockets(&thisClient->socks) < 0)
    {
        printf("libhtttp/client createClient: socket creation failed\n");
        return -1;
    }
    if (connectOnTCP(thisClient->socks, ipAddress) < 0)
    {
        printf("libhtttp/client joinLobby: failed to connect to server at IP\n");
        return -1;
    }
    if (registerWithServer(thisClient))
    {
        printf("libhtttp/client joinLobby: failed to register\n");
        return -1;
    }
    return 0;
}
