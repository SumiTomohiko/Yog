#include <stdio.h>

struct Foo {
    unsigned int bar: 31;
};

void
test_ubit_31(struct Foo* foo)
{
    printf("%u", foo->bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
