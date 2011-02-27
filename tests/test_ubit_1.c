#include <stdio.h>

struct Foo {
    unsigned int bar: 1;
};

void
test_ubit_1(struct Foo* foo)
{
    printf("%u", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
