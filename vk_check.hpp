#include <stdio.h>

#define VK_CHECK(expr) do { if(expr != VK_SUCCESS) { fprintf(stderr, "Vulkan pooped itself:%s\n", #expr); abort(); } } while(0)

