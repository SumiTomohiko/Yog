#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
print_pointer(const void* p)
{
    if (p == NULL) {
        printf("(nil)");
        return;
    }
    printf("%p", p);
}

void
print_string2(const char* s)
{
    print_pointer(s);
}

void
print_string(const char* s)
{
    printf("%s", s);
}

struct Baz {
    char* buf;
};

void
test_Buffer0(struct Baz* baz)
{
    strcpy(baz->buf, "bar");
}

void
test_int_p(int* p)
{
    *p = 42;
}

struct Bar {
    const char* baz;
};

void
test_string(struct Bar* bar)
{
    bar->baz = "foobarbazquux";
}

struct Foo {
    int bar;
};

void
print_struct(struct Foo* foo)
{
    printf("%d", foo->bar);
}

struct Hoge {
};

void
print_struct2(struct Hoge* hoge)
{
    print_pointer(hoge);
}

void
foo()
{
    printf("42");
}

void
print_uint8(uint8_t n)
{
    printf("%hhu", n);
}

void
print_int8(int8_t n)
{
    printf("%hhd", n);
}

void
print_uint16(uint16_t n)
{
    printf("%hu", n);
}

void
print_int16(int16_t n)
{
    printf("%hd", n);
}

void
print_uint32(uint32_t n)
{
    printf("%u", n);
}

void
print_int32(int32_t n)
{
    printf("%d", n);
}

void
print_uint64(uint64_t n)
{
    printf("%" PRIu64, n);
}

void
print_int64(int64_t n)
{
    printf("%" PRId64, n);
}

void
print_float(float f)
{
    printf("%.2f", f);
}

void
print_double(double f)
{
    printf("%.2f", f);
}

void
print_uchar(unsigned char n)
{
    printf("%hhu", n);
}

void
print_char(signed char n)
{
    printf("%hhd", n);
}

void
print_ushort(unsigned short n)
{
    printf("%hu", n);
}

void
print_short(short n)
{
    printf("%hd", n);
}

void
print_uint(unsigned int n)
{
    printf("%u", n);
}

void
print_int(int n)
{
    printf("%d", n);
}

void
print_ulong(unsigned long n)
{
    printf("%lu", n);
}

void
print_long(long n)
{
    printf("%ld", n);
}

void
print_longdouble(long double f)
{
    printf("%.2Lf", f);
}

void
print_three_int(int n, int m, int l)
{
    printf("%d%d%d", n, m, l);
}

void
print_two_int(int n, int m)
{
    printf("%d%d", n, m);
}

const char*
return_string()
{
    return "foobarbazquux";
}

uint8_t
return_uint8()
{
    return 42;
}

int8_t
return_int8()
{
    return 42;
}

uint16_t
return_uint16()
{
    return 42;
}

int16_t
return_int16()
{
    return 42;
}

uint32_t
return_uint32_0()
{
    return 1073741823;
}

uint32_t
return_uint32_1()
{
    return 1073741824;
}

int32_t
return_int32_0()
{
    return -1073741825;
}

int32_t
return_int32_1()
{
    return -1073741824;
}

int32_t
return_int32_2()
{
    return 1073741823;
}

int32_t
return_int32_3()
{
    return 1073741824;
}

uint64_t
return_uint64_0()
{
    return 1073741823;
}

uint64_t
return_uint64_1()
{
    return 1073741824;
}

int64_t
return_int64_0()
{
    return -4611686018427387905LL;
}

int64_t
return_int64_1()
{
    return -4611686018427387904LL;
}

int64_t
return_int64_2()
{
    return -1073741825;
}

int64_t
return_int64_3()
{
    return -1073741824;
}

int64_t
return_int64_4()
{
    return 1073741823;
}

int64_t
return_int64_5()
{
    return 1073741824;
}

int64_t
return_int64_6()
{
    return 4611686018427387903LL;
}

int64_t
return_int64_7()
{
    return 4611686018427387904LL;
}

float
return_float()
{
    return 3.14;
}

double
return_double()
{
    return 3.14;
}

unsigned char
return_uchar()
{
    return 42;
}

signed char
return_char()
{
    return 42;
}

unsigned short
return_ushort()
{
    return 42;
}

short
return_short()
{
    return 42;
}

unsigned int
return_uint_0()
{
    return 1073741823;
}

unsigned int
return_uint_1()
{
    return 1073741824;
}

int
return_int_0()
{
    return -1073741825;
}

int
return_int_1()
{
    return -1073741824;
}

int
return_int_2()
{
    return 1073741823;
}

int
return_int_3()
{
    return 1073741824;
}

unsigned long
return_ulong_0()
{
    return 1073741823;
}

unsigned long
return_ulong_1()
{
    return 1073741824;
}

long
return_long_0()
{
    return -1073741825;
}

long
return_long_1()
{
    return -1073741824;
}

long
return_long_2()
{
    return 1073741823;
}

long
return_long_3()
{
    return 1073741824;
}

long double
return_long_double()
{
    return 3.14;
}

void*
return_pointer_0()
{
    return (void*)42;
}

struct Foo*
return_pointer_10()
{
    struct Foo* foo = (struct Foo*)malloc(sizeof(struct Foo));
    foo->bar = 42;
    return foo;
}

struct Foo*
return_pointer_20()
{
    return NULL;
}

void
test_pointer_p(struct Foo** p)
{
    *p = (struct Foo*)malloc(sizeof(struct Foo));
    (*p)->bar = 42;
}

struct Quux {
    char foo[0];
};

void
test_pointer_p2(struct Quux** p)
{
    *p = (struct Quux*)malloc(sizeof(struct Quux) + sizeof(char) * 1);
    (*p)->foo[0] = 42;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
