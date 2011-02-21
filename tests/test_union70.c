#include <stdio.h>
#include "test_union70.h"

void
test_union70(struct Foo* foo)
{
    printf("%d", foo->bar.baz);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
