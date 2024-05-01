#include <criterion/criterion.h>

#include <stdio.h>

#include "limit_tree.h"

struct limit_tree tree;

void limit_tree_setup_bid(void) {
  tree = limit_tree_new(SIDE_BID);
}

void limit_tree_setup_ask(void) {
  tree = limit_tree_new(SIDE_ASK);
}

void limit_tree_teardown(void) {
  limit_tree_free(&tree);
}

Test(limit_tree,
     add_right,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  struct limit* limit1 = malloc(sizeof(struct limit));
  *limit1 = (struct limit){.price = 1};
  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->price, 1);

  struct limit* limit2 = malloc(sizeof(struct limit));
  *limit2 = (struct limit){.price = 2};
  limit_tree_add(&tree, limit2);
  cr_assert_eq(tree.size, 2);
  cr_assert_eq(tree.root->right->price, 2);

  struct limit* limit3 = malloc(sizeof(struct limit));
  *limit3 = (struct limit){.price = 3};
  limit_tree_add(&tree, limit3);
  cr_assert_eq(tree.size, 3);
  cr_assert_eq(tree.root->right->right->price, 3);
}

Test(limit_tree,
     add_left,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  struct limit* limit1 = malloc(sizeof(struct limit));
  *limit1 = (struct limit){.price = 3};
  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->price, 3);

  struct limit* limit2 = malloc(sizeof(struct limit));
  *limit2 = (struct limit){.price = 2};
  limit_tree_add(&tree, limit2);
  cr_assert_eq(tree.size, 2);
  cr_assert_eq(tree.root->left->price, 2);

  struct limit* limit3 = malloc(sizeof(struct limit));
  *limit3 = (struct limit){.price = 1};
  limit_tree_add(&tree, limit3);
  cr_assert_eq(tree.size, 3);
  cr_assert_eq(tree.root->left->left->price, 1);
}

Test(limit_tree,
     add_left_right,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  struct limit* limit1 = malloc(sizeof(struct limit));
  *limit1 = (struct limit){.price = 2};
  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->price, 2);

  struct limit* limit2 = malloc(sizeof(struct limit));
  *limit2 = (struct limit){.price = 1};
  limit_tree_add(&tree, limit2);
  cr_assert_eq(tree.size, 2);
  cr_assert_eq(tree.root->left->price, 1);

  struct limit* limit3 = malloc(sizeof(struct limit));
  *limit3 = (struct limit){.price = 3};
  limit_tree_add(&tree, limit3);
  cr_assert_eq(tree.size, 3);
  cr_assert_eq(tree.root->right->price, 3);
}

Test(limit_tree,
     add_existing,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  struct limit* limit1 = malloc(sizeof(struct limit));
  *limit1 = (struct limit){.price = 2};
  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->price, 2);

  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->left, NULL);
  cr_assert_eq(tree.root->right, NULL);
}

Test(limit_tree,
     remove_leaf,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  uint64_t prices[5] = {20, 32, 15, 12, 17};
  struct limit* limits[5] = {NULL, NULL, NULL, NULL, NULL};

  for (int i = 0; i < 5; i++) {
    limits[i] = malloc(sizeof(struct limit));
    *limits[i] = (struct limit){.price = prices[i]};
    limit_tree_add(&tree, limits[i]);
    cr_assert_eq(tree.size, i + 1);
  }

  cr_assert_eq(limit_tree_min(&tree)->price, 12);
  limit_tree_remove(&tree, limits[3]);
  cr_assert_eq(tree.size, 4);
  cr_assert_eq(limit_tree_min(&tree)->price, 15);

  cr_assert_eq(limit_tree_max(&tree)->price, 32);
  limit_tree_remove(&tree, limits[1]);
  cr_assert_eq(tree.size, 3);
  cr_assert_eq(limit_tree_max(&tree)->price, 20);
}

void inorder_accumulate(struct limit* node, int* i, struct limit** buffer) {
  if (node == NULL)
    return;

  inorder_accumulate(node->left, i, buffer);

  // printf("%ld ", node->price);
  buffer[(*i)++] = node;

  inorder_accumulate(node->right, i, buffer);
}

static int cmp(struct limit** a, struct limit** b) {
  if ((*a)->price == (*b)->price)
    return 0;
  return 1;
}

Test(limit_tree,
     remove_node_one_child,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  uint64_t prices[6] = {20, 32, 15, 12, 17, 33};
  struct limit* limits[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

  for (int i = 0; i < 6; i++) {
    limits[i] = malloc(sizeof(struct limit));
    *limits[i] = (struct limit){.price = prices[i]};
    limit_tree_add(&tree, limits[i]);
    cr_assert_eq(tree.size, i + 1);
  }

  struct limit* expected[] = {limits[3], limits[2], limits[4],
                              limits[0], limits[1], limits[5]};
  struct limit** buffer = malloc(sizeof(struct limit*) * 6);
  int i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected, 6, cmp);

  limit_tree_remove(&tree, limits[1]);
  cr_assert_eq(tree.size, 5);

  struct limit* expected2[] = {limits[3], limits[2], limits[4], limits[0],
                               limits[5]};
  i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected2, 5, cmp);

  struct limit* limit = malloc(sizeof(struct limit));
  *limit = (struct limit){.price = 32};
  limit_tree_add(&tree, limit);
  cr_assert_eq(tree.size, 6);

  struct limit* expected3[] = {limits[3], limits[2], limits[4],
                               limits[0], limit,     limits[5]};
  i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected3, 6, cmp);

  limit_tree_remove(&tree, limit);
  cr_assert_eq(tree.size, 5);

  i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected2, 5, cmp);

  free(buffer);
}

Test(limit_tree,
     remove_node_two_child,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  uint64_t prices[6] = {20, 32, 15, 12, 17, 33};
  struct limit* limits[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

  for (int i = 0; i < 6; i++) {
    limits[i] = malloc(sizeof(struct limit));
    *limits[i] = (struct limit){.price = prices[i]};
    limit_tree_add(&tree, limits[i]);
    cr_assert_eq(tree.size, i + 1);
  }

  struct limit* expected[] = {limits[3], limits[2], limits[4],
                              limits[0], limits[1], limits[5]};
  struct limit** buffer = malloc(sizeof(struct limit*) * 6);
  int i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected, 6, cmp);

  limit_tree_remove(&tree, limits[2]);
  cr_assert_eq(tree.size, 5);

  struct limit* expected2[] = {limits[3], limits[4], limits[0], limits[1],
                               limits[5]};
  i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected2, 5, cmp);

  limit_tree_remove(&tree, limits[3]);
  cr_assert_eq(tree.size, 4);

  struct limit* expected3[] = {limits[4], limits[0], limits[1], limits[5]};
  i = 0;
  inorder_accumulate(tree.root, &i, buffer);
  cr_assert_arr_eq_cmp(buffer, expected3, 4, cmp);

  free(buffer);
}

Test(limit_tree,
     min,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  struct limit* limit1 = malloc(sizeof(struct limit));
  *limit1 = (struct limit){.price = 3};
  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->price, 3);

  struct limit* limit2 = malloc(sizeof(struct limit));
  *limit2 = (struct limit){.price = 2};
  limit_tree_add(&tree, limit2);
  cr_assert_eq(tree.size, 2);
  cr_assert_eq(tree.root->left->price, 2);

  struct limit* limit3 = malloc(sizeof(struct limit));
  *limit3 = (struct limit){.price = 4};
  limit_tree_add(&tree, limit3);
  cr_assert_eq(tree.size, 3);
  cr_assert_eq(tree.root->right->price, 4);

  cr_assert_eq(limit_tree_min(&tree)->price, 2);

  struct limit* limit4 = malloc(sizeof(struct limit));
  *limit4 = (struct limit){.price = 1};
  limit_tree_add(&tree, limit4);
  cr_assert_eq(tree.size, 4);
  cr_assert_eq(tree.root->left->left->price, 1);

  cr_assert_eq(limit_tree_min(&tree)->price, 1);
}

Test(limit_tree,
     max,
     .init = limit_tree_setup_bid,
     .fini = limit_tree_teardown) {
  cr_assert_eq(tree.size, 0);

  struct limit* limit1 = malloc(sizeof(struct limit));
  *limit1 = (struct limit){.price = 3};
  limit_tree_add(&tree, limit1);
  cr_assert_eq(tree.size, 1);
  cr_assert_eq(tree.root->price, 3);

  struct limit* limit2 = malloc(sizeof(struct limit));
  *limit2 = (struct limit){.price = 2};
  limit_tree_add(&tree, limit2);
  cr_assert_eq(tree.size, 2);
  cr_assert_eq(tree.root->left->price, 2);

  struct limit* limit3 = malloc(sizeof(struct limit));
  *limit3 = (struct limit){.price = 4};
  limit_tree_add(&tree, limit3);
  cr_assert_eq(tree.size, 3);
  cr_assert_eq(tree.root->right->price, 4);

  cr_assert_eq(limit_tree_max(&tree)->price, 4);

  struct limit* limit4 = malloc(sizeof(struct limit));
  *limit4 = (struct limit){.price = 5};
  limit_tree_add(&tree, limit4);
  cr_assert_eq(tree.size, 4);
  cr_assert_eq(tree.root->right->right->price, 5);

  cr_assert_eq(limit_tree_max(&tree)->price, 5);
}
