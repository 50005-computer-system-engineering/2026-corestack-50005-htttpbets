#ifndef LIBHTTTP_CLIENTLL_H
#define LIBHTTTP_CLIENTLL_H

#include "common.h"

// structs
struct Node {
    struct Node *next;
    Endpoint client;
};
typedef struct Node ClientNode;

typedef struct {
    ClientNode *head;
    uint32_t count;
} ClientLinkedList;

// list functions
int addToList(ClientLinkedList *list, Endpoint *client);
int removeFromList(ClientLinkedList *list, uint32_t id);
int getFromList(ClientLinkedList *list, Endpoint **client, uint32_t id);
int freeList(ClientLinkedList **listToFree);

#endif