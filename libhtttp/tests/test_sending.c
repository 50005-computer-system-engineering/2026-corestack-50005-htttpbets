#include "libhtttp.h"

int main(void)
{
    printf("test_sending.c start\nforking into server and client to send a message\n");

    int pid = fork();

    if (pid < 0)
    {
        perror("test_libhtttp.c fork");
        return -1;
    }

    // server fork
    if (pid > 0)
    {
        // listening for 1 client
        Connection *socks = malloc(sizeof(Connection));
        if (createSockets(socks) < 0) 
        {
            printf("server: failed to create sockets\n");
            goto fail;
        }
        if(listenOnServer(socks) < 0)
        {
            printf("server: failed to start server listener\n");
            goto fail;
        }
        Record *clientArray = malloc(sizeof(Record));
        if (clientArray == NULL)
        {
            perror("server malloc");
            goto fail;
        }
        if (openLobbyOnServer(socks, clientArray, 1) < 0)
        {
            printf("server: failed to accept clients\n");
            goto fail;
        }

        // listen for a message
        Message *theMessage = malloc(sizeof(Message));
        if (receiveMessage(clientArray[0].connection.tcp, &theMessage) < 0)
        {
            printf("server: failed to receive message\n");
            goto fail;
        }
        
        // printing out the message
        printf("server: received this message\n\tsourceId: %u\n\ttype: %u\n\tlength: %u\n\tcontent: %s\n", theMessage->sourceId, theMessage->type, theMessage->length, theMessage->content);

        // finish up
        printf("server: no probrem\n");
        wait(NULL);
        closeSockets(socks);
        return 0;
    
    fail:
        printf("server: has problem\n");
        wait(NULL);
        closeSockets(socks);
        return -1;
    }

    // client fork
    if (pid == 0)
    {
        // connecting to the server
        Connection *socks = malloc(sizeof(Connection));
        if (createSockets(socks) < 0) 
        {
            printf("client: failed to create sockets\n");
            exit(-1);
        }
        // TODO replace with proper synchronisation
        sleep(1);
        int server = connectToServer(socks, "127.0.0.1");
        if (server < 0)
        {
            printf("client: failed to connect to server server\n");
            exit(-1);
        }

        sleep(1);
        // building a message
        Message theMessage = {
            .sourceId = 10,
            .type = 20,
            .length = 11,
            .content = malloc(11)
        };
        theMessage.content = (unsigned char *)"gamer word";

        // sending it
        printf("client: sending message...\n");
        sendMessage(socks->tcp, theMessage);

        // TODO replace with proper synchronisation
        sleep(1);
        printf("client: no probrem\n");
        closeSockets(socks);
        exit(0);
    }
}