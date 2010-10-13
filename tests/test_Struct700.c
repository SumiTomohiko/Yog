
struct Foo {
    int bar;
};

struct Baz {
    struct Foo foo;
    int quux;
};

void
test_Struct700(struct Baz* baz)
{
    baz->quux = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
