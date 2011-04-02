#include <stdio.h>

struct Foo {
    int bar;
};

void
test_Struct840(struct Foo foo)
{
    printf("%d", foo.bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
