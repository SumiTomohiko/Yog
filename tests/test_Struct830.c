
struct Foo {
    unsigned int bar: 9;
    char baz;
};

void
test_Struct830(struct Foo* foo)
{
    foo->baz = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
