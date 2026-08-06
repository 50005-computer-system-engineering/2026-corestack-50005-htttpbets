#ifndef LIBBR_CLIENT_H
#define LIBBR_CLIENT_H

#include <stdint.h>

typedef void BRClient;

int brclient_init(BRClient **clientPtr);
int brclient_join(BRClient *clientPtr, char *ipAddress);
// int leaveLobby(LibhtttpClient *clientPtr);

int brclient_send_msg(BRClient *clientPtr, unsigned char content[512]); // use defined value instead of explicit number

int brclient_get_app_msg(unsigned char returnMsg[512]);

#endif