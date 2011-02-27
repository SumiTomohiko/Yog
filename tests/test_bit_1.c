#include <stdio.h>

struct Foo {
    int bar: 1;
};

void
test_bit_1(struct Foo* foo)
{
    printf("%d", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
