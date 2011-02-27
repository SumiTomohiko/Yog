#include <stdio.h>

struct Foo {
    unsigned int bar: 2;
};

void
test_ubit_2(struct Foo* foo)
{
    printf("%u", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
