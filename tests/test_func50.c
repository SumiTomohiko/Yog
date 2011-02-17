#include <assert.h>
#include <stdlib.h>
#include "test_func50.h"

Foo*
test_func50()
{
    Foo* foo = (Foo*)malloc(sizeof(Foo));
    assert(foo != NULL);
    foo->bar = 42;
    return foo;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
