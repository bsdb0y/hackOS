#include "memory.h"

void* memset(void* ptr, int c, size_t size) {
    char* c_ptr = (char*)ptr;
    for (int j = 0; j < size; ++j) {
        c_ptr[j] = (char)c;
    }
    return ptr;
}