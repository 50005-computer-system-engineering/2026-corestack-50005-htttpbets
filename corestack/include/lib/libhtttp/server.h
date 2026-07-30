#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void LibhtttpServer;

int createServer(LibhtttpServer **serverPtr);
int openLobby(LibhtttpServer *serverPtr)
int closeLobby(LibhtttpServer *serverPtr);

int listenForClientMsg(LibhtttpServer *serverPtr, uint32_t *sourceId, unsigned char **returnMsg);
int sendBroadcastToClients(LibhtttpServer *serverPtr, uint32_t length, unsigned char *content);

#endif