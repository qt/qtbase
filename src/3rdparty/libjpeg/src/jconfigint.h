// Definitions for building in Qt source, ref. src/jconfigint.h.in

#include <stdint.h>

#define BUILD ""

#define INLINE inline

#define PACKAGE_NAME "libjpeg-turbo"

#define VERSION "3.0.0"

#if SIZE_MAX == 0xffffffff
#define SIZEOF_SIZE_T 4
#elif SIZE_MAX == 0xffffffffffffffff
#define SIZEOF_SIZE_T 8
#endif

#if defined(_MSC_VER) && defined(HAVE_INTRIN_H)
#if (SIZEOF_SIZE_T == 8)
#define HAVE_BITSCANFORWARD64
#elif (SIZEOF_SIZE_T == 4)
#define HAVE_BITSCANFORWARD
#endif
#endif

#define FALLTHROUGH

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8      /* use 8 or 12 */
#endif

#undef C_ARITH_CODING_SUPPORTED
#undef D_ARITH_CODING_SUPPORTED
#undef WITH_SIMD

#if BITS_IN_JSAMPLE == 8

/* Support arithmetic encoding */
#define C_ARITH_CODING_SUPPORTED 1

/* Support arithmetic decoding */
#define D_ARITH_CODING_SUPPORTED 1

/* Use accelerated SIMD routines. */
/* #undef WITH_SIMD */

#endif
