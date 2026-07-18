#ifndef LIBHTTTP_CLIENT_H
#define LIBHTTTP_CLIENT_H

typedef void LibhtttpClient;

int createClient(LibhtttpClient **clientPtr);
int joinLobby(LibhtttpClient *clientPtr, char *ipAddress);
// int leaveLobby(LibhtttpClient *clientPtr);

#endif