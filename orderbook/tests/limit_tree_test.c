#include <criterion/criterion.h>

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
