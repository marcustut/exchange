#ifndef LIMIT_TREE_H
#define LIMIT_TREE_H

#include "limit.h"
#include "uint64_hashmap.h"

struct limit_tree {
  enum side side;      // indicate whether bid / ask
  struct limit* best;  // the best limit level (best bid / ask)
  struct uint64_hashmap price_limit_map;  // keeping track of price levels

  struct limit* root;  // root of the limit tree
  uint64_t size;       // total limits in the tree
};

struct limit_tree limit_tree_new(enum side side);
void limit_tree_free(struct limit_tree* tree);
void limit_tree_update_best(struct limit_tree*, struct limit*);
void limit_tree_add(struct limit_tree*, struct limit*);
void limit_tree_remove(struct limit_tree*, struct limit*);
struct limit* limit_tree_min(struct limit_tree*);
struct limit* limit_tree_max(struct limit_tree*);

#endif