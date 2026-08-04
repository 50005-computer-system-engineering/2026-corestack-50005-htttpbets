#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void BRServer;

int createServer(BRServer **serverPtr);
int openLobby(BRServer *serverPtr);
int startGame(BRServer *serverPtr);
int endGame(BRServer *serverPtr);

int sendBroadcastToClients(BRServer *serverPtr, uint32_t length, unsigned char *content);

#endif