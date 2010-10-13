
struct Foo {
    int bar;
};

struct Baz {
    int quux;
    struct Foo foo;
};

void
test_Struct705(struct Baz* baz)
{
    baz->foo.bar = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
