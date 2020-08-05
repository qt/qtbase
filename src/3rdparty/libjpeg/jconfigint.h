// Definitions for building in Qt source, ref. src/jconfigint.h.in

#include <stdint.h>

#define BUILD ""

#define INLINE inline

#define PACKAGE_NAME "libjpeg-turbo"

#define VERSION "2.0.5"

#if SIZE_MAX == 0xffffffff
#define SIZEOF_SIZE_T 4
#elif SIZE_MAX == 0xffffffffffffffff
#define SIZEOF_SIZE_T 8
#endif
