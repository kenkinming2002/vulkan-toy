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

  void delegate_chain_init(DelegateChain& delegate_chain);
  void delegate_chain_register(DelegateChain& delegate_chain, Delegate& delegate);
  void delegate_chain_deregister(Delegate& delegate);
  void delegate_chain_invoke(DelegateChain& delegate_chain);
}
