#include <stdio.h>

struct Foo {
    int bar: 31;
};

void
test_bit_31(struct Foo* foo)
{
    printf("%d", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
