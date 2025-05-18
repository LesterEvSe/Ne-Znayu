#include "queue.h"

// TODO Think about GC for every actor with this queue.
// Change all malloc and free function to memory.h macroses

MessageQueue *new_queue() {
  Node *dummy = (Node*)malloc(sizeof(Node));
  dummy->data = NULL;

  MessageQueue *q = (MessageQueue*)malloc(sizeof(MessageQueue));
  atomic_store(&q->head, dummy);
  atomic_store(&q->tail, dummy);
  return q;
}

void free_queue(MessageQueue *q) {
  /*
  Node *current = atomic_load(&q->head);
  while (current != NULL) {
    Node *next = atomic_load(&current->next);
    free(current);  // TODO change free function
    current = next;
  }
  free(q);
  */
}

void add(MessageQueue *q, ActorData *data) {
  /*
  Node *new_node = (Node*)malloc(sizeof(Node));
  new_node->data = (ActorData*)malloc(sizeof(ActorData));
    
  new_node->data->message = data->message;  // because of string interning
  new_node->data->arg_count = data->arg_count;

    
  new_node->data->slots = (Value*)malloc(data->arg_count * sizeof(Value));
  for (int i = 0; i < data->arg_count; ++i) {
    new_node->data->slots[i].type = data->slots[i].type;
  }
  */
}

Node *get(MessageQueue *q) {
  return NULL;
}