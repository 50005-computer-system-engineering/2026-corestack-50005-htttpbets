#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void BRServer;

int createServer(BRServer **serverPtr);
int openLobby(BRServer *serverPtr);
int startGame(BRServer *serverPtr);
int endGame(BRServer *serverPtr);

int sendAppMessageToClient(BRServer *serverPtr, uint32_t clientId, unsigned char content[512]); // use defined value instead of explicit number
int sendBroadcastToClients(BRServer *serverPtr, unsigned char content[512]);

#endif