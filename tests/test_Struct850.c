#include <stdio.h>

struct Foo {
    int bar;
};

struct Baz {
    struct Foo foo;
};

void
test_Struct850(struct Baz baz)
{
    printf("%d", baz.foo.bar);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
