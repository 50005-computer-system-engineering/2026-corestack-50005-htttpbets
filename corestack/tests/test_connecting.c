#include "lib/libhtttp.h"

int main(void) 
{
    int lobbySize = 2;
    printf("test_connecting.c start\nforking into server and client tests with %d clients\n", lobbySize);
    int pid = fork();
    if (pid < 0)
    {
        perror("test_libhtttp.c fork");
        return -1;
    }
    // server fork
    if (pid > 0) 
    {
        Connection *socks = malloc(sizeof(Connection));
        if (createSockets(socks) < 0) 
        {
            printf("server: failed to create sockets\n");
            return -1;
        }
        if(listenOnServer(socks) < 0)
        {
            printf("server: failed to start server listener\n");
            return -1;
        }
        Record *clientArray = malloc(sizeof(Record) * lobbySize);
        if (!(clientArray))
        {
            perror("server malloc");
        }
        if (openLobbyOnServer(socks, clientArray, lobbySize) < 0)
        {
            printf("server: failed to accept clients");
            return -1;
        }
        for (int i = 0; i < lobbySize; i++)
        {
            printf("server: checking client array\n\tid = %d\n\ttoken = %d\n\tconnection.tcp = %d\n", clientArray[i].id, clientArray[i].token, clientArray[i].connection.tcp);
        }
        printf("server: no probrem\n");
        wait(NULL);
        closeSockets(socks);
        return 0;  
    }

    // client fork
    if (pid == 0)
    {
        int pid = fork();
        if (pid < 0)
        {
            perror("client fork");
            printf("client: forking into 2 failed");
            exit(-1);
        }
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
        printf("client: no probrem\n");
        closeSockets(socks);
        if (pid > 0)
        {
            wait(NULL);
        }
        exit(0);
    }
}
