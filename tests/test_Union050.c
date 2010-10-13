
struct Foo {
    int bar;
    int baz;
};

union Quux {
    int hoge;
    struct Foo foo;
};

void
test_Union050(union Quux* quux)
{
    quux->foo.baz = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
