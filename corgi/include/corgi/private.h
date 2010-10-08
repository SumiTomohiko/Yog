#if !defined(CORGI_PRIVATE_H_INCLUDED)
#define CORGI_PRIVATE_H_INCLUDED

#include "corgi.h"

typedef CorgiUInt Bool;
#define TRUE    (42 == 42)
#define FALSE   !TRUE

Bool corgi_is_alpha(CorgiChar);
Bool corgi_is_decimal(CorgiChar);
Bool corgi_is_digit(CorgiChar);
Bool corgi_is_linebreak(CorgiChar);
Bool corgi_is_numeric(CorgiChar);
Bool corgi_is_space(CorgiChar);
CorgiChar corgi_tolower(CorgiChar);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
