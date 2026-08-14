#ifndef LIBHTTTP_SERVER_H
#define LIBHTTTP_SERVER_H

#include <stdint.h>

typedef void BRServer;

int brserver_init(BRServer** server_ptr);
int brserver_init_room(BRServer** server_ptr, uint16_t tcp_port, uint32_t id_base);
int brserver_open(BRServer* server_ptr);
int brserver_start(BRServer* server_ptr);
int brserver_end(BRServer* server_ptr);

int brserver_client_info(BRServer* server_ptr, uint32_t* n_clients, uint32_t* client_ids);

int brserver_send_to_target(BRServer* server_ptr, uint32_t client_id, unsigned char content[1024]); // use defined value instead of explicit number
int brserver_send_broadcast(BRServer* server_ptr, unsigned char content[1024]);
int brserver_send_to_all(BRServer* server_ptr, unsigned char content[1024]);

int brserver_get_app_msg(unsigned char return_msg[1024]);

#endif
