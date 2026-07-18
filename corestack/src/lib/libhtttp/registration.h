#ifndef LIBHTTTP_SOMETHING_H
#define LIBHTTTP_SOMETHING_H

#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// new client functions
int registerNewClient(Endpoint *newClient);
int registerWithServer(Endpoint *myClient);

// // admin action functions
// int adminActionOnClient(int sockfd, Endpoint **Endpoint);
// int handleKickFromServer(int sockfd, Endpoint **Endpoint);

#endif