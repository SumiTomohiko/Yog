/*
   Unicode character type helpers.

   Written by Marc-Andre Lemburg (mal@lemburg.com).
   Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

   Copyright (c) Corporation for National Research Initiatives.

*/
#include "corgi/config.h"
#include "corgi.h"
#include "corgi/private.h"

typedef unsigned char Flags;
#define ALPHA_MASK      0x01
#define DECIMAL_MASK    0x02
#define DIGIT_MASK      0x04
#define NODELTA_MASK    0x08
#define NUMERIC_MASK    0x10

struct Entry {
    CorgiChar lower;
    Flags flags;
};

typedef struct Entry Entry;

Entry table[] = {
#include "entries.inc"
};

static CorgiUInt
compute_index(CorgiChar c)
{
    if (0x110000 <= c) {
        return 0;
    }
#include "indexes.inc"
    CorgiUInt index = index1[c >> SHIFT];
    return index2[(index << SHIFT) + (c & ((1 << SHIFT) - 1))];
}

static const Entry*
get_entry(CorgiChar c)
{
    return &table[compute_index(c)];
}

static Bool
is_flag(CorgiChar c, Flags flag)
{
    return get_entry(c)->flags & flag ? TRUE : FALSE;
}

Bool
corgi_is_linebreak(CorgiChar c)
{
    switch (c) {
#include "linebreaks.inc"
        return TRUE;
    default:
        return FALSE;
    }
}

Bool
corgi_is_space(CorgiChar c)
{
    switch (c) {
#include "whitespaces.inc"
        return TRUE;
    default:
        return FALSE;
    }
}

Bool
corgi_is_decimal(CorgiChar c)
{
    return is_flag(c, DECIMAL_MASK);
}

Bool
corgi_is_digit(CorgiChar c)
{
    return is_flag(c, DIGIT_MASK);
}

Bool
corgi_is_numeric(CorgiChar c)
{
    return is_flag(c, NUMERIC_MASK);
}

Bool
corgi_is_alpha(CorgiChar c)
{
    return is_flag(c, ALPHA_MASK);
}

CorgiChar
corgi_tolower(CorgiChar c)
{
    const Entry* entry = get_entry(c);
    int delta = entry->lower;
    if (entry->flags & NODELTA_MASK) {
        return delta;
    }
    return 32768 <= delta ? c + delta - 65536 : c + delta;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
