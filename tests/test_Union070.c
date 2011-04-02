#include <stdio.h>

struct Foo {
    int bar;
};

union Baz {
    struct Foo foo;
};

void
test_Union070(union Baz baz)
{
    printf("%d", baz.foo.bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
