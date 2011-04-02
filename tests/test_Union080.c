#include <stdio.h>

union Foo {
    int bar;
};

union Baz {
    union Foo foo;
};

void
test_Union080(union Baz baz)
{
    printf("%d", baz.foo.bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
