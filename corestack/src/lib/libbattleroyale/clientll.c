#include "clientll.h"

int initaliseList(ClientLinkedList **list)
{
    // check pointer
    if (*list == NULL)
    {
        LOG_D("[initaliseList()] cleaning list");
        free(*list);
    }

    // make a new list
    ClientLinkedList *newList = malloc(sizeof(ClientLinkedList));
    if (newList == NULL)
    {
        perror("[initialiseList()] malloc");
        LOG_E("[initialiseList()] could not allocate memory for linked list");
        return -1;
    }

    // default values 
    newList->count = 0;
    newList->head = NULL;

    //return to pointer
    *list = newList;

    LOG_I("[initaliseList()] initalised list successfully");

    return 0;
}

int addToList(ClientLinkedList *list, Endpoint *client)
{
    // list might be full (unlikely)
    if (list->count == UINT32_MAX)
    {
        LOG_E("[addToList()] list is somehow full, failed to add");
        return -1;
    }
    ClientNode *slot = NULL;
    // case where list is empty
    if (list->head == NULL)
    {
        LOG_D("[addToList()] list is empty, creating new head");
        list->head = malloc(sizeof(ClientNode));
        // new client node with new client
        slot = list->head;
    }
    // case where list is populated
    else
    {
        LOG_D("[addToList()] list is populated, finding next available slot");
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
    LOG_I("[clientll addToList()] new client added to list, list now has %u clients", list->count);
    return 0;
}

int removeFromList(ClientLinkedList *list, uint32_t id)
{
    // check for empty list
    if (list->head == NULL)
    {
        LOG_E("[removeFromList()] list is empty, nothing to remove");
        return -1;
    }
    // check every next node until new 
    ClientNode *checking = NULL;
    checking = list->head;
    // head is the target
    if (checking->client.id == id)
    {
        LOG_D("[removeFromList()] match is current list head");
        list->head = checking->next;
        free(checking);
        checking = NULL;
        list->count--;
        return 0;
    }
    // any other node is target
    while (checking->next != NULL)
    {
        // match id
        if (checking->next->client.id == id)
        {
            LOG_D("[removeFromList()] match is the next client in list");
            ClientNode *removedNode = checking->next;
            checking->next = checking->next->next;
            free(removedNode);
            list->count--;
            return 0;
        }
    }
    LOG_E("[removeFromList()] id could not be matched in list");
    return -1;        
}

int getFromList(ClientLinkedList *list, Endpoint *returnClient, uint32_t id)
{
    // check for emply list
    if (list->head == NULL)
    {
        LOG_E("[getFromList()] list is empty, nothing to remove");
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
            LOG_E("[getFromList()] id matched, breaking search");
            goto found;
        }
        checking = checking->next;
    }
    LOG_E("[getFromList()] id could not be matched in list");
    returnClient = NULL;
    return -1;

    // return struct through pointer
    found:
    *returnClient = checking->client;
    LOG_I("[getFromList()] client assigned to return buffer");
    return 0;
}

int getIdArray(ClientLinkedList *list, uint32_t **ids)
{
    // check for empty list
    LOG_I("[getIdArray()] creating id array from client linked list...");
    if (list->head == NULL)
    {
        LOG_E("[getIdArray()] list is empty, nothing to remove");
        return -1;
    }
    // allocate space
    *ids = malloc(sizeof(uint32_t) * list->count);
    // iterate until NULL
    uint32_t *currentId = *ids;
    ClientNode *currentNode = list->head;
    do {
        *currentId = currentNode->client.id;
        currentNode = currentNode->next;
        currentId++;
    } while(currentNode != NULL);
    LOG_I("[getIdArray()] id array creation complete");
    return 0;
}

int freeList(ClientLinkedList **listToFree)
{
    // check for empty list
    ClientLinkedList *list = *listToFree;
    if (list->head == NULL)
    {
        LOG_E("[freeList()] list is empty, freeing pointer");
        goto freeing;
    }
    // loop through and free individual endpoints (also closing the socks)
    ClientNode *current = NULL;
    ClientNode *next = list->head;
    do
    {
        // initialise loop
        current = next;
        next = current->next;
        LOG_D("[freeList()] removing ClientNode for client %u", current->client.id);
        // close connections
        if (closeSockets(current->client.socks) < 0)
        {
            LOG_E("[freeList()] could not close socket connections with client %u", current->client.id);
            return -1;
        }
        // free current ClientNode
        free(current);
        current = NULL;
    // continue if there is still a next node
    } while (next != NULL);
    LOG_D("[freeList()] all ClientNodes freed, ready to free list");
    // free ClientLinkedList struct
    freeing:
    free(list);
    list = NULL;
    LOG_I("[freeList()] list has been freed");

    return 0;
}
