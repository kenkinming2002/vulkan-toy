#pragma once

#include "utils/ll.hpp"

namespace vulkan
{
  struct DelegateChain
  {
    struct ll delegates;
  };

  struct Delegate
  {
    struct ll_node node;

    void(*ptr)(void *);
    void *data;
  };

  inline void delegate_chain_init(DelegateChain& delegate_chain)
  {
    ll_init(&delegate_chain.delegates);
  }

  inline void delegate_chain_register(DelegateChain& delegate_chain, Delegate& delegate)
  {
    ll_append(&delegate_chain.delegates, &delegate.node);
  }

  inline void delegate_chain_deregister(Delegate& delegate)
  {
    ll_remove(&delegate.node);
  }

  inline void delegate_chain_invoke(DelegateChain& delegate_chain)
  {
    for(struct ll_node *curr = ll_front(&delegate_chain.delegates);
        curr != ll_sentinel(&delegate_chain.delegates);
        curr = curr->next)
    {
      Delegate *delegate = (Delegate *)curr;
      delegate->ptr(delegate->data);
    }
  }
}
