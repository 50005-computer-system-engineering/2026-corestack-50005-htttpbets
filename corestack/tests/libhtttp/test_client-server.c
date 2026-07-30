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
        if (openLobby(server) < 0)
        {
            printf("server: could not open lobby\n");
            return -1;
        }
        printf("server: successfully accepted 2 clients\n");

        sleep(5);

        if (startGame(server) < 0)
        {
            printf("server: could not start listening\n");
            return -1;
        }
        printf("server: started listening for messages\n");

        wait(NULL);

        printf("server: test finished\n");

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

        sleep(15);
        uint32_t msgLen = 8;
        unsigned char *msgContent = (unsigned char *)"game ON";
        if (sendAsClient(client1, msgLen, msgContent) < 0)
        {
            printf("client1: could not send message\n");
            exit(-1);
        }
        printf("client1: sent message\n");

        sleep(2);
        msgLen = 9;
        msgContent = (unsigned char *)"bird OFF";
        if (sendAsClient(client2, msgLen, msgContent) < 0)
        {
            printf("client2: could not send message\n");
            exit(-1);
        }
        printf("client2: sent message\n");
    }
}
