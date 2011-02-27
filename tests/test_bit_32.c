#include <stdio.h>

struct Foo {
    int bar: 32;
};

void
test_bit_32(struct Foo* foo)
{
    printf("%d", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
