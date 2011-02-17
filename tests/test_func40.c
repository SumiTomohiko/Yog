#include <assert.h>
#include <stdlib.h>
#include "test_func40.h"

struct Foo*
test_func40()
{
    struct Foo* foo = (struct Foo*)malloc(sizeof(struct Foo));
    assert(foo != NULL);
    foo->bar = 42;
    return foo;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
