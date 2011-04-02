#include <stdio.h>

union Foo {
    int bar;
};

void
test_Union060(union Foo foo)
{
    printf("%d", foo.bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
