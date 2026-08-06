#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void BRServer;

int brserver_init(BRServer **serverPtr);
int brserver_open(BRServer *serverPtr);
int brserver_start(BRServer *serverPtr);
int brserver_end(BRServer *serverPtr);

int brserver_client_info(BRServer *serverPtr, uint32_t *nClients, uint32_t *clientIds);

int brserver_send_to_target(BRServer *serverPtr, uint32_t clientId, unsigned char content[512]); // use defined value instead of explicit number
int brserver_send_broadcast(BRServer *serverPtr, unsigned char content[512]);
int brserver_send_to_all(BRServer *serverPtr, unsigned char content[512]);

int brserver_get_app_msg(unsigned char returnMsg[512]);

#endif