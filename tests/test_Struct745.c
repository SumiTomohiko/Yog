
struct Foo {
    int bar;
};

struct Foo foo;

struct Baz {
    struct Foo* foo;
};

struct Baz baz;

struct Baz*
test_Struct745()
{
    foo.bar = 42;
    baz.foo = &foo;

    return &baz;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
