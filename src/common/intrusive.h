#ifndef MINITCP_SRC_COMMON_INTRUSIVE_H_
#define MINITCP_SRC_COMMON_INTRUSIVE_H_

#include "logging.h"

namespace minitcp {

// some useful macros for intrusive link lists.
#define offset_of(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#define container_of(POINTER, TYPE, MEMBER)                    \
  ({                                                           \
    const typeof(((TYPE *)0)->MEMBER) *__mpointer = (POINTER); \
    (TYPE *)((char *)__mpointer - offset_of(TYPE, MEMBER));    \
  })

struct list_head {
  struct list_head *prev, *next;
};

static inline int list_empty(struct list_head *list) {
  return list->next == list;
}

static inline void list_insert_impl(struct list_head *node,
                                    struct list_head *prev,
                                    struct list_head *next) {
  node->prev = prev;
  node->next = next;

  prev->next = node;
  next->prev = node;
}

static inline void list_insert_before(struct list_head *node,
                                      struct list_head *prev) {
  MINITCP_ASSERT(prev) << " intrusive list insert before : prev is nullptr."
                       << std::endl;
  MINITCP_ASSERT(prev->prev)
      << "intrusive list insert before : prev->prev is nullptr." << std::endl;

  list_insert_impl(node, prev->prev, prev);
}

static inline void list_insert_after(struct list_head *node,
                                     struct list_head *next) {
  MINITCP_ASSERT(next) << "intrusive list insert after : next is nullptr."
                       << std::endl;
  MINITCP_ASSERT(next->next)
      << "intrusive list insert after : next->next is nullptr." << std::endl;

  list_insert_impl(node, next, next->next);
}

static inline void list_insert(struct list_head *list, struct list_head *node) {
  MINITCP_ASSERT(list) << "intrusive list insert : list is nullptr."
                       << std::endl;

  list_insert_impl(node, list, list->next);
}

static inline void list_remove_impl(struct list_head *prev,
                                    struct list_head *next) {
  prev->next = next;
  next->prev = prev;
}

static inline void list_remove(struct list_head *node) {
  MINITCP_ASSERT(node->prev)
      << " intrusive list remove : node->prev is nullptr." << std::endl;
  MINITCP_ASSERT(node->next)
      << " intrusive list remove : node->next is nullptr." << std::endl;
  list_remove_impl(node->prev, node->next);
}

}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_INTRUSIVE_H_