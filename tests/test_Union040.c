
union Foo {
    int bar;
    int baz;
};

void
test_Union040(union Foo* foo)
{
    foo->baz = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
