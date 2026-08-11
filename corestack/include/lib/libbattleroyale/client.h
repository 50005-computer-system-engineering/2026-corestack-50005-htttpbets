#ifndef LIBBR_CLIENT_H
#define LIBBR_CLIENT_H

#include <stdint.h>

typedef void BRClient;

// States to identify client state
typedef enum
{
    BRCLIENT_STATE_IDLE = 0,  // Connected but not doing anything yet
    BRCLIENT_STATE_LOBBY = 1, // Waiting in lobby for game to start
    BRCLIENT_STATE_GAME = 2,  // Actively playing a match
    BRCLIENT_STATE_END = 3    // Match ended
} BRClientState;

int brclient_init(BRClient **clientPtr);
int brclient_join(BRClient *clientPtr, char *ipAddress);
int brclient_get_state(BRClient *clientPtr);
// int leaveLobby(LibhtttpClient *clientPtr);

int brclient_send_msg(BRClient *clientPtr, unsigned char content[512]); // use defined value instead of explicit number

int brclient_get_app_msg(unsigned char returnMsg[512]);

int brclient_get_id(BRClient *clientPtr, uint32_t *id); // returns 0 on success, -1 on failure; writes id via out-param

#endif