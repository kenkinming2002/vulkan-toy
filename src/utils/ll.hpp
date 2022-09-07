#pragma once

struct ll_node
{
  struct ll_node *prev;
  struct ll_node *next;
};

inline void ll_insert_before(struct ll_node *pos, struct ll_node *node)
{
  node->prev = pos->prev;
  node->next = pos;

  node->prev->next = node;
  node->next->prev = node;
}

inline void ll_insert_after(struct ll_node *pos, struct ll_node *node)
{
  node->prev = pos;
  node->next = pos->next;

  node->prev->next = node;
  node->next->prev = node;
}

inline void ll_remove(struct ll_node *node)
{
  node->prev->next = node->next;
  node->next->prev = node->prev;

  node->next = node->prev = nullptr;
}

struct ll
{
  struct ll_node sentinel;
};

inline void ll_init(struct ll *ll)
{
  ll->sentinel.prev = ll->sentinel.next = &ll->sentinel;
}

inline struct ll_node *ll_front(struct ll *ll) { return ll->sentinel.next; }
inline struct ll_node *ll_back(struct ll *ll)  { return ll->sentinel.prev; }
inline struct ll_node *ll_sentinel(struct ll *ll)  { return &ll->sentinel; }

inline void ll_prepend(struct ll *ll, struct ll_node *node) { ll_insert_before(ll_front(ll), node); }
inline void ll_append(struct ll *ll, struct ll_node *node)  { ll_insert_after(ll_back(ll), node);   }
