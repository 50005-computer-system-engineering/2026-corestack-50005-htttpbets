#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "libhtttp/server.h"
#include "libhtttp/client.h"

int main(void)
{
    printf("splitting into server and client forks for testing...\n");
    int pid = fork();

    // if ((child1 < 0) || (child1 > 0 && child2 < 0))
    // {
    //     perror("fork");
    //     return -1;
    // }

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
        
        if (openLobby(server, 2) < 0)
        {
            printf("server: could not open lobby\n");
            return -1;
        }
        printf("server: successfully accepted 2 clients\n");

        // unsigned char *buffer = NULL;
        // if (listenForClientMsg(server, &buffer) < 0)
        // {
        //     printf("server: could not read message\n");
        //     return -1;
        // }

        // printf("server: received message %s\n", buffer);

        wait(NULL);

        return 0;
    }

    // client 
    if (pid == 0)
    {
        sleep(1);
        LibhtttpClient *client1 = NULL;
        
        if (createClient(&client1) < 0)
        {
            printf("client1: could not create client\n");
            exit(-1);
        }
        if (joinLobby(client1, "127.0.0.1") < 0)
        {
            printf("client1: could not join lobby\n");
            exit(-1);
        }
        // sleep(5);
        // uint32_t msgLen = 11;
        // unsigned char *msgContent = (unsigned char *)"gamer word";
        // if (sendAsClient(client, msgLen, msgContent) < 0)
        // {
        //     printf("client1: could not send message\n");
        //     exit(-1);
        // }
        printf("client1: joined lobby without issue\n");

        // second client
        LibhtttpClient *client2 = NULL;
        if (createClient(&client2) < 0)
        {
            printf("client2: could not create client\n");
            exit(-1);
        }
        if (joinLobby(client2, "127.0.0.1") < 0)
        {
            printf("client2: could not join lobby\n");
            exit(-1);
        }
        printf("client2: joined lobby without issue\n");

        exit(0);
    }
}

