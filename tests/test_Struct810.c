
struct Foo {
    unsigned int bar: 8;
    char baz;
};

void
test_Struct810(struct Foo* foo)
{
    foo->baz = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
