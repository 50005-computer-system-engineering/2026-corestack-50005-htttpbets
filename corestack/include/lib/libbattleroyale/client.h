#ifndef LIBBR_CLIENT_H
#define LIBBR_CLIENT_H

#include <stdint.h>

typedef void BRClient;

int createClient(BRClient **clientPtr);
int joinLobby(BRClient *clientPtr, char *ipAddress);
// int leaveLobby(LibhtttpClient *clientPtr);

int sendAppMessage(BRClient *clientPtr, unsigned char content[512]); // use defined value instead of explicit number

#endif