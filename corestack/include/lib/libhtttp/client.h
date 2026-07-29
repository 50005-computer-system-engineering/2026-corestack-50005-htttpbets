#ifndef LIBHTTTP_CLIENT_H
#define LIBHTTTP_CLIENT_H

#include <stdint.h>

typedef void LibhtttpClient;

int createClient(LibhtttpClient **clientPtr);
int joinLobby(LibhtttpClient *clientPtr, char *ipAddress);
// int leaveLobby(LibhtttpClient *clientPtr);

int sendAsClient(LibhtttpClient *clientPtr, uint32_t length, unsigned char *content);
int receiveBroadcastAsClient(LibhtttpClient *clientPtr, unsigned char **returnBuffer);

#endif