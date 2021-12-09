#include "intrusive.h"

#include <gtest/gtest.h>
using namespace minitcp;

TEST(intrusive, create) { struct list_head head = {&head, &head}; }

TEST(intrusive, insert_2_node) {
  struct list_head head = {&head, &head};
  struct list_head node = {nullptr, nullptr};
  list_insert(&head, &node);

  EXPECT_EQ(head.next, &node);
  EXPECT_EQ(node.prev, &head);
  EXPECT_EQ(head.prev, &node);
  EXPECT_EQ(node.next, &head);
}

TEST(intrusive, insert_3_node) {
  struct list_head head = {&head, &head};
  struct list_head node1 = {nullptr, nullptr};
  struct list_head node2 = {nullptr, nullptr};

  list_insert(&head, &node2);
  list_insert(&head, &node1);

  EXPECT_EQ(head.next, &node1);
  EXPECT_EQ(node1.next, &node2);
  EXPECT_EQ(node2.next, &head);
}

TEST(intrusive, insert_after) {
  struct list_head head = {&head, &head};
  struct list_head node1 = {nullptr, nullptr};
  struct list_head node2 = {nullptr, nullptr};

  list_insert(&head, &node1);
  list_insert_after(&node2, &node1);

  EXPECT_EQ(node1.next, &node2);
  EXPECT_EQ(head.prev, &node2);
}

TEST(intrusive, insert_before) {
  struct list_head head = {&head, &head};
  struct list_head node1 = {nullptr, nullptr};
  struct list_head node2 = {nullptr, nullptr};

  list_insert(&head, &node2);
  list_insert_before(&node1, &node2);

  EXPECT_EQ(head.next, &node1);
  EXPECT_EQ(node2.prev, &node1);
}

TEST(intrusive, remove) {
  struct list_head list = {&list, &list};
  struct list_head node1 = {nullptr, nullptr};
  struct list_head node2 = {nullptr, nullptr};

  list_insert(&list, &node2);
  list_insert(&list, &node1);

  list_remove(&node1);

  EXPECT_EQ(list.next, &node2);
  EXPECT_EQ(node2.next, &list);

  EXPECT_EQ(list.prev, &node2);
  EXPECT_EQ(node2.prev, &list);
}