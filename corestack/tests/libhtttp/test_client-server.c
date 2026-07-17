#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhtttp/server.h"
#include "libhtttp/client.h"

int main(void)
{
    printf("splitting into server and client forks for testing...\n");
    int pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    // server fork
    if (pid > 0)
    {
        LibhtttpServer *server = NULL;

        if (createServer(&server) < 0)
        {
            printf("server: could not create server\n");
            return -1;
        }
        printf("server: successfully created server pointer\n");
        
        if (openLobby(server, 1) < 0)
        {
            printf("server: could not open lobby\n");
            return -1;
        }
        printf("server: successfully accepted 1 client\n");

        
    }

    // client
    if (pid == 0)
    {
        LibhtttpClient *client = NULL;
        if (createClient(&client) < 0)
        {
            printf("client: could not create client\n");
            exit(-1);
        }
        if (joinLobby(client, "127.0.0.1") < 0)
        {
            printf("client: could not join lobby\n");
            exit(-1);
        }
        printf("client: joined lobby without issue\n");
        exit(0);
    }
}

