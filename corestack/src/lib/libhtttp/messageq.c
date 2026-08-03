#include "message.h"
#include "messageq.h"

int queueSize(MessageQueue *q)
{
    int length = 0;
    
    // no members
    if (q->front == NULL && q->back == NULL)
    {
        return length; // still 0
    }

    // has 1 member
    if (q->front == q->back)
    {
        return ++length; // 1
    }

    // has more than 1
    struct QueueMember current = q->front;
    do
    {
        length++;
        current = current.next
    } while (current != NULL)
    return length;
}

/*
function adds to back of queue
returns -1 to indicate failure
*/
int enqueue(MessageQueue *q, Message *msg)
{
    struct QueueMember *newMember = malloc(sizeof(struct QueueMember));
    if (newMember == NULL)
    {
        perror("[enqueue()] malloc");
        return -1;
    }
    
    // if empty list
    if (queueSize(q) == 0)
    {
        newMember->next = NULL;
        newMember->prev = NULL;
        q->front = newMember;
        q->back = newMember;
        LOG_D("[enqueue()] queued new message to an empty queue");
        return 0;
    }

    // there are existing members
    q->back->next = newMember;
    q->back = newMember;
    LOG_D("[enqueue()] queued new message to queue");
    return 0;
}

/*
Returns to pointer the front of queue
*/
void peek(MessageQueue *q, Message *msg)
{
    // if list is empty
    if (queueSize == 0)
    {
        msg = NULL; // returns null
        return -1;
    }

    // if queue has only 1 member
    if (queueSize == 1)
    {
        msg = q->front->msg;
        return 0;
    }

    // if queue has more than 1 member
    msg = q->front->msg;
    return 0;
}

/*
function removes from front of the queue
returns to pointer
returns 0 if pointer has Message, -1 if there is none
*/
void dequeue(MessageQueue *q, Message *msg)
{
    // if list is empty
    if (queueSize == 0)
    {
        msg = NULL; // returns null
        return -1;
    }

    // if queue has only 1 member
    if (queueSize == 1)
    {
        msg = q->front->msg;
        q->front = NULL;
        q->back = NULL;
        return 0;
    }

    // if queue has more than 1 member
    msg = q->front->msg;
    q->front = q->front->next;
    q->front->prev = NULL;
    return 0;
}
