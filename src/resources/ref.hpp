#pragma once

#define container_of(ptr, type, member) \
    reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(ptr) - offsetof(type, member))

namespace vulkan
{
  // Reference: https://nullprogram.com/blog/2015/02/17/

  typedef struct Ref *ref_t;

  struct Ref
  {
    unsigned count;
    void(*free)(ref_t res);
  };

  inline void ref_get(ref_t ref)
  {
    ++ref->count;
  }

  inline void ref_put(ref_t ref)
  {
    if(--ref->count == 0)
      ref->free(ref);
  }
}
