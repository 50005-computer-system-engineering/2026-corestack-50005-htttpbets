#include "clientll.h"

int addToList(ClientLinkedList *list, Endpoint *client)
{
    // list might be full (unlikely)
    if (list->count == UINT32_MAX)
    {
        printf("[clientll addToList()] list is somehow full, failed to add\n");
        return -1;
    }
    ClientNode *slot = NULL;
    // case where list is empty
    if (list->head == NULL)
    {
        printf("[clientll addToList()] list is empty, creating new head\n");
        list->head = malloc(sizeof(ClientNode));
        // new client node with new client
        slot = list->head;
    }
    // case where list is populated
    else
    {
        printf("[clientll addToList()] list is populated, finding next available slot\n");
        // find next available slot
        ClientNode *lastSlot = list->head;
        while (lastSlot->next != NULL)
        {
            lastSlot = lastSlot->next;
        }
        lastSlot->next = malloc(sizeof(ClientNode));
        slot = lastSlot->next;
    }
    // new client node with new client
    slot->next = NULL;
    slot->client = *client;
    list->count++;
    // update 
    printf("[clientll addToList()] new client added to list, list now has %u clients\n", list->count);
    return 0;
}

int removeFromList(ClientLinkedList *list, uint32_t id)
{
    // check for emply list
    if (list->head == NULL)
    {
        printf("[clientll removeFromList()] list is empty, nothing to remove\n");
        return -1;
    }
    // check every next node until new 
    ClientNode *checking = NULL;
    checking = list->head;
    if (checking->client.id == id)
    {
        goto removal;
    }
    while (checking->next != NULL)
    {
        // match id
        if (checking->next->client.id == id)
        {
            goto removal;
        }
    }
    printf("[clientll removeFromList()] id could not be matched in list\n");
    return -1;
    removal:
        printf("[clientll removeFromList()] matching client found\n");
        ClientNode *removedNode = checking->next;
        checking->next = checking->next->next;
        free(removedNode);
        return 0;
}

int getFromList(ClientLinkedList *list, Endpoint **returnClient, uint32_t id)
{
    // check for emply list
    if (list->head == NULL)
    {
        printf("[clientll getFromList()] list is empty, nothing to remove\n");
        return -1;
    }
    // check every next node until new 
    ClientNode *checking = NULL;
    checking = list->head;
    while (checking->next != NULL)
    {
        // match id
        if (checking->client.id == id)
        {
            printf("[clientll getFromList()] id matched, breaking search\n");
            goto found;
        }
        checking = checking->next;
    }
    printf("[clientll getFromList()] id could not be matched in list\n");
    *returnClient = malloc(sizeof(1));
    return -1;

    // return struct through pointer
    found:
    *returnClient = &checking->client;
    printf("[clientll getFromList()] client assigned to return buffer\n");
    return 0;
}

int freeList(ClientLinkedList **listToFree)
{
    // check for empty list
    *list = *listToFree;
    if (*list->head == NULL)
    {
        printf("[clientll getFromList()] list is empty, freeing pointer\n");
        free(*list)
        return 0;
    }
    // loop through and free individual endpoints (also closing the socks)
    ClientNode *nextUp = NULL
    do
    {
        // TODO actually close
    } while (nextUp != NULL)
    return 0;
}