#include "limit_tree.h"

#include <stdlib.h>

struct limit_tree limit_tree_new(enum side side) {
  return (struct limit_tree){.side = side, .map = price_limit_map_new()};
}

/**
 * Free all limits in tree using an in-order traversal
 */
void _limit_tree_free_limits(struct limit* node) {
  if (node == NULL)
    return;

  _limit_tree_free_limits(node->left);
  limit_free(node);
  _limit_tree_free_limits(node->right);
  free(node);
}

void limit_tree_free(struct limit_tree* tree) {
  // Since all limits are on the heap, we free them using a traversal
  _limit_tree_free_limits(tree->root);
  price_limit_map_free(&tree->map);
}

void limit_tree_update_best(struct limit_tree* tree, struct limit* limit) {
  if (limit == NULL) {
    if (tree->best == NULL)
      return;

    if (tree->size == 0) {  // no more limits hence no best limit
      tree->best = NULL;
      return;
    }

    if (tree->best->order_head == NULL) {  // no more orders in queue
      tree->best = NULL;
      return;
    }

    switch (tree->side) {
      case SIDE_BID:
        tree->best = limit_tree_max(tree);
        break;
      case SIDE_ASK:
        tree->best = limit_tree_min(tree);
        break;
    }
  }

  if (tree->best == NULL) {
    tree->best = limit;
    return;
  }

  switch (tree->side) {
    case SIDE_BID:
      if (limit->price > tree->best->price)
        tree->best = limit;
      break;
    case SIDE_ASK:
      if (limit->price < tree->best->price)
        tree->best = limit;
      break;
  }
}

struct limit* _limit_tree_add(struct limit* node, struct limit* limit) {
  if (node == NULL) {
    node = limit;
    return node;
  }

  if (limit->price > node->price)  // traverse right
    node->right = _limit_tree_add(node->right, limit);
  else if (limit->price < node->price)  // traverse left
    node->left = _limit_tree_add(node->left, limit);

  return node;
}

void limit_tree_add(struct limit_tree* tree, struct limit* limit) {
  if (tree->root == NULL) {  // tree don't have limits yet
    tree->root = limit;
    tree->size++;
    return;
  }

  if (limit->price > tree->root->price)  // traverse right
    tree->root->right = _limit_tree_add(tree->root->right, limit);
  else if (limit->price < tree->root->price)  // traverse left
    tree->root->left = _limit_tree_add(tree->root->left, limit);
  else  // already exist
    return;

  tree->size++;
}

struct limit* _limit_tree_remove(struct limit* node, struct limit* limit) {
  if (node == NULL)
    return node;

  if (limit->price > node->price)  // traverse right
    node->right = _limit_tree_remove(node->right, limit);
  else if (limit->price < node->price)  // traverse left
    node->left = _limit_tree_remove(node->left, limit);
  else {  // found
    // Case 1: only one child or no child
    if (node->left == NULL)
      return node->right;
    else if (node->right == NULL)
      return node->left;

    // Case 2: has both left and right child (replace by left)
    struct limit* left = node->left;
    struct limit* right = node->right;
    node = left;
    node->right = right;
  }

  return node;
}

void limit_tree_remove(struct limit_tree* tree, struct limit* limit) {
  tree->root = _limit_tree_remove(tree->root, limit);
  free(limit);
  tree->size--;
}

struct limit* limit_tree_min(struct limit_tree* tree) {
  struct limit* node = tree->root;

  // get the leftmost node
  while (node->left != NULL)
    node = node->left;

  return node;
}

struct limit* limit_tree_max(struct limit_tree* tree) {
  struct limit* node = tree->root;

  // get the rightmost node
  while (node->right != NULL)
    node = node->right;

  return node;
}