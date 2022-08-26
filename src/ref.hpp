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

#define REF_DECLARE(type,ptr_type) \
  typedef struct type *ptr_type;   \
  ref_t as_ref(ptr_type ptr);      \
  void get(ptr_type ptr);          \
  void put(ptr_type ptr);

#define REF_DEFINE(type,ptr_type,member)              \
  ref_t as_ref(ptr_type ptr) { return &ptr->member; } \
  void get(ptr_type ptr) { ref_get(as_ref(ptr)); }    \
  void put(ptr_type ptr) { ref_put(as_ref(ptr)); }

}
