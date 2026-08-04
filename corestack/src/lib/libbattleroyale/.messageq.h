#ifndef LIBHTTTP_MESSAGEQ_H
#define LIBHTTTP_MESSAGEQ_H

#include "message.h"

struct QueueMember {
    Message *msg;
    struct QueueMember *next;
    struct QueueMember *prev;
};

typedef struct {
    struct QueueMember *front;
    struct QueueMember *back;
} MessageQueue;

int queueSize(MessageQueue *q);
int enqueue(MessageQueue *q, Message *msg);
int peek(MessageQueue *q, Message **msg);
int dequeue(MessageQueue *q, Message **msg);

#endif