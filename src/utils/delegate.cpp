#include "delegate.hpp"

namespace vulkan
{
  void delegate_chain_init(DelegateChain& delegate_chain)
  {
    ll_init(&delegate_chain.delegates);
  }

  void delegate_chain_register(DelegateChain& delegate_chain, Delegate& delegate)
  {
    ll_append(&delegate_chain.delegates, &delegate.node);
  }

  void delegate_chain_deregister(Delegate& delegate)
  {
    ll_remove(&delegate.node);
  }

  void delegate_chain_invoke(DelegateChain& delegate_chain)
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
