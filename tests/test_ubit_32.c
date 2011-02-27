#include <stdio.h>

struct Foo {
    unsigned int bar: 32;
};

void
test_ubit_32(struct Foo* foo)
{
    printf("%u", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
