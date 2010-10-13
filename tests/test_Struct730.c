
struct Foo {
    struct {
        int bar;
    } baz;
    int quux;
};

void
test_Struct730(struct Foo* foo)
{
    foo->quux = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
