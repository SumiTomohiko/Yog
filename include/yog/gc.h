#ifndef __YOG_GC_H__
#define __YOG_GC_H__

#include <stdlib.h>

struct Heap {
    size_t size;
    void* ptr;
    void* free;
    struct Heap* next;
};

typedef struct Heap Heap;

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
