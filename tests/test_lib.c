#include <stdint.h>
#include <stdio.h>

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
    printf("%llu", n);
}

void
print_int64(int64_t n)
{
    printf("%lld", n);
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
print_pointer(void* ptr)
{
    printf("%p", ptr);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
