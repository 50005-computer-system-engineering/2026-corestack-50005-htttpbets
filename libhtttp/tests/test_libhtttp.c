#include "libhtttp.h"

int main(void) {
    Connection socks;
    if (createSockets(&socks) == -1) 
    {
        printf("failed to create servers\n");
        return -1;
    }
    if(listenOnServer(&socks) == -1)
    {
        printf("failed to start server listener\n");
        return -1;
    }
    printf("no probrem\n");
    return 0;
}