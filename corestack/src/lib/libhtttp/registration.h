#ifndef LIBHTTTP_SOMETHING_H
#define LIBHTTTP_SOMETHING_H

#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "connection.h"
#include "message.h"

// new client functions
int registerNewClient(Record *newClient);
int registerWithServer(Record *myClient);

// // admin action functions
// int adminActionOnClient(int sockfd, Record **Record);
// int handleKickFromServer(int sockfd, Record **Record);

#endif