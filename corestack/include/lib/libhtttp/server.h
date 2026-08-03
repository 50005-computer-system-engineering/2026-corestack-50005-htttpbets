#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void LibhtttpServer;

int createServer(LibhtttpServer **serverPtr);
int openLobby(LibhtttpServer *serverPtr);
int startGame(LibhtttpServer *serverPtr);
int endGame(LibhtttpServer *serverPtr);

int sendBroadcastToClients(LibhtttpServer *serverPtr, uint32_t length, unsigned char *content);

#endif