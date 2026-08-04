#ifndef LIBBR_CLIENT_H
#define LIBBR_CLIENT_H

#include <stdint.h>

typedef void BRClient;

int createClient(BRClient **clientPtr);
int joinLobby(BRClient *clientPtr, char *ipAddress);
// int leaveLobby(LibhtttpClient *clientPtr);

int sendAsClient(BRClient *clientPtr, uint32_t length, unsigned char *content);
int receiveBroadcastAsClient(BRClient *clientPtr, unsigned char **returnBuffer);

#endif