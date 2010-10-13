
union Foo {
    int bar;
    int baz;
};

void
test_Union030(union Foo* foo)
{
    foo->bar = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
