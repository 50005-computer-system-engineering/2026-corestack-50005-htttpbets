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
        
        uint32_t *clientIdArray;
        uint32_t size = 2;
        if (openLobby(server, &size, &clientIdArray) < 0)
        {
            printf("server: could not open lobby\n");
            return -1;
        }
        printf("server: successfully accepted 2 clients\n");

        unsigned char *buffer = NULL;
        uint32_t source1 = 0;
        uint32_t source2 = 0;

        if (listenForClientMsg(server, &source1, &buffer) < 0)
        {
            printf("server: could not read message\n");
            return -1;
        }
        printf("server: received first message from client %u : %s\n", source1, buffer);
        free(buffer);
        buffer = NULL;

        if (listenForClientMsg(server, &source2, &buffer) < 0)
        {
            printf("server: could not read message\n");
            return -1;
        }
        printf("server: received second message from client %u : %s\n", source2, buffer);
        free(buffer);
        buffer = NULL;

        wait(NULL);
        printf("test complete\n");
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

        sleep(5);
        uint32_t msgLen = 11;
        unsigned char *msgContent = (unsigned char *)"gamer word";
        if (sendAsClient(client1, msgLen, msgContent) < 0)
        {
            printf("client1: could not send message\n");
            exit(-1);
        }
        printf("client1: sent message\n");

        sleep(5);
        msgLen = 11;
        msgContent = (unsigned char *)"gamer word";
        if (sendAsClient(client2, msgLen, msgContent) < 0)
        {
            printf("client2: could not send message\n");
            exit(-1);
        }
        printf("client2: sent message\n");

        exit(0);
    }
}

