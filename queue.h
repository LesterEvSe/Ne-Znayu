#ifndef PL_QUEUE_H
#define PL_QUEUE_H

#include <stdatomic.h>
#include <stdlib.h>

#include "common.h"
#include "actor_vm.h"


// Node of doubly linked list
typedef struct Node {
  ActorData *data;
  _Atomic(struct Node*) next;
} Node;

// Based on the doubly linked list data structure
typedef struct MessageQueue {
  _Atomic(Node*) head;
  _Atomic(Node*) tail;
} MessageQueue;


MessageQueue *new_queue();
void free_queue(MessageQueue *q);
void add(MessageQueue *q, ActorData *data);
Node *get(MessageQueue *q);

#endif // PL_QUEUE_H