
struct Foo {
    unsigned int bar: 31;
    unsigned int baz: 2;
};

void
test_Struct400(struct Foo* foo)
{
    foo->baz = 3;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
