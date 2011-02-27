
struct Foo {
    unsigned int bar: 1;
    unsigned int baz: 1;
};

void
test_Struct770(struct Foo* foo)
{
    foo->baz = 1;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
