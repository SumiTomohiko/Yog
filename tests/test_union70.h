struct Foo {
    union {
        int baz;
    } bar;
};
void test_union70(struct Foo*);
