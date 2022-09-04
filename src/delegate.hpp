#pragma once

namespace vulkan
{
  struct List
  {
    List *prev, *next;
  };

  inline void list_init(List& list) { list.prev = list.next = &list; }

  inline void list_remove(List& list)
  {
    list.prev->next = list.next;
    list.next->prev = list.prev;
    list.prev = list.next = nullptr;
  }

  inline void list_append(List& list, List& value)
  {
    value.prev = list.prev;
    value.next = &list;

    value.prev->next = &value;
    value.next->prev = &value;
  }

  struct DelegateChain
  {
    List delegates;
  };

  struct Delegate
  {
    List list;

    void(*ptr)(void *);
    void *data;
  };

  inline void delegate_chain_init(DelegateChain& delegate_chain)
  {
    list_init(delegate_chain.delegates);
  }

  inline void delegate_chain_register(DelegateChain& delegate_chain, Delegate& delegate)
  {
    list_append(delegate_chain.delegates, delegate.list);
  }

  inline void delegate_chain_deregister(Delegate& delegate)
  {
    list_remove(delegate.list);
  }

  inline void delegate_chain_invoke(DelegateChain& delegate_chain)
  {
    for(List *curr = delegate_chain.delegates.next; curr != &delegate_chain.delegates; curr = curr->next)
    {
      Delegate *delegate = (Delegate *)curr;
      delegate->ptr(delegate->data);
    }
  }
}
