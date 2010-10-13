
struct Foo {
    struct {
        int bar;
    } baz;
};

void
test_Struct720(struct Foo* foo)
{
    foo->baz.bar = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
