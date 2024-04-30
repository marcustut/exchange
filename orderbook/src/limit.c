#include "limit.h"

#include <stdlib.h>

void limit_free(struct limit* limit) {
  // free the order queue (a linked list)
  if (limit->order_head != NULL) {
    struct order* curr = limit->order_head;
    while (curr != NULL) {
      struct order* next = curr->next;
      free(curr);
      curr = next;
    }
  }
}

struct limit limit_default() {
  return (struct limit){};
}