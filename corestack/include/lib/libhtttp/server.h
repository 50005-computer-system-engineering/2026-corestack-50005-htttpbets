#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void LibhtttpServer;

int createServer(LibhtttpServer **serverPtr);
int openLobby(LibhtttpServer *serverPtr, uint8_t lobbySize);
int closeLobby(LibhtttpServer *serverPtr);

int listenForClientMsg(LibhtttpServer *serverPtr, unsigned char **returnMsg);

#endif