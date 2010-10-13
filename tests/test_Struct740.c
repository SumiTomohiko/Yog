
struct Foo {
    int bar;
};

struct Baz {
    struct Foo* foo;
};

void
test_Struct740(struct Baz* baz)
{
    baz->foo->bar = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
