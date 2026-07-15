#include "connection.h"
#include "message.h"

#include <sys/wait.h>
#include <unistd.h>

// TODO proper assert statements with Unity framework

int main(void) 
{
    // fork into server (parent) & client (child)
    int pid = fork();
    if (pid < 0)
    {
        perror("test_basics fork");
    }

    // test createSockets()
    Connection *socks = malloc(sizeof(Connection));
    if (createSockets(&socks) < 0)
    {
        printf("test_basics: failed to allocate sockets\n");
        goto fail;
    }

    // server tests
    if (pid > 0)
    {
        // listen for connection
        if (listenOnTCP(socks) < 0)
        {
            printf("test_basics server: listenOnTCP failed\n");
            goto fail;
        }
        // accept client
        int clientFd = acceptOnTCP(socks);
        if (clientFd < 0)
        {
            printf("test_basics server: acceptOnTCP failed\n");
            goto fail;    
        }
        // wait for client to terminate

        printf("test_basics server: connection.h no problems\n");

        // try receive from client
        Message *msgFrom = malloc(sizeof(Message)); 
        if (receiveMessage(clientFd, &msgFrom) < 0)
        {
            printf("test_basics server: receiveMessage failed\n");
            goto fail;
        }

        printf("test_basics server: message.h no problems\n");

        wait(NULL);
        // close own sockets
        closeSockets(socks);
        socks = NULL;

        return 0;
    }

    if (pid == 0)
    {
        // TODO proper sync between procs
        sleep(1);
        // connect to server
        if (connectOnTCP(socks, "127.0.0.1") < 0)
        {
            printf("test_basics client: acceptOnTCP failed\n");
            closeSockets(socks);
            exit(-1);
        }
        printf("test_basics client: connection.h no problems\n");

        sleep(1);
        // build message and send
        Message msgTo = {
            .sourceId = (uint8_t)10,
            .type = (uint8_t)100,
            .length = (uint32_t)11,
            .content = malloc(11)
        };
        if (msgTo.content == NULL)
        {
            perror("client malloc");
            exit(-1);
        }
        msgTo.content = (unsigned char *)"gamer word"; 
        if (sendMessage(socks->tcp, msgTo) < 0)
        {
            printf("test_basics client: sendMessage failed\n");
        }
        printf("test_basics client: message.h no problems\n");

        exit(0);
    }

fail:
    wait(NULL);
    closeSockets(socks);
    socks = NULL;
    return -1;
}
