#include <stdio.h>

union Foo {
    int bar;
};

struct Baz {
    union Foo foo;
};

void
test_Struct860(struct Baz baz)
{
    printf("%d", baz.foo.bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
