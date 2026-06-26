#include "libhtttp.h"

int main(void) {
    printf("test_libhtttp.c start\nforking into server and client tests\n");
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
        int newClient = acceptOnServer(socks);
        if (newClient < 0)
        {
            printf("server: failed to accept new clients\n");
            return -1;
        }
        printf("server: no probrem\n");
        closeSockets(socks);
        return 0;
    }

    // client fork
    if (pid == 0)
    {
        Connection *socks = malloc(sizeof(Connection));
        if (createSockets(socks) < 0) 
        {
            printf("client: failed to create sockets\n");
            return -1;
        }
        sleep(1); //TODO replace with proper synchronisation
        int server = connectToServer(socks, "127.0.0.1");
        if (server < 0)
        {
            printf("client: failed to connect to server server\n");
            return -1;
        }
        printf("client: no probrem\n");
        closeSockets(socks);
        return 0;
    }
    
}