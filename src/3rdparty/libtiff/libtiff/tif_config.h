/*
  Configuration defines for Qt.
*/

#include <qglobal.h>
#if defined(Q_OS_WINCE)
# include <qfunctions_wince.h>
#endif

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Support CCITT Group 3 & 4 algorithms */
#define CCITT_SUPPORT 1

/* Pick up YCbCr subsampling info from the JPEG data stream to support files
   lacking the tag (default enabled). */
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1

/* Support C++ stream API (requires C++ compiler) */
/* #undef CXX_SUPPORT */

/* Treat extra sample as alpha (default enabled). The RGBA interface will
   treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many
   packages produce RGBA files but don't mark the alpha properly. */
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1

/* Use the Apple OpenGL framework. */
/* #undef HAVE_APPLE_OPENGL_FRAMEWORK */

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#if !defined(Q_OS_WINCE)
#define HAVE_FCNTL_H 1
#endif

/* Define to 1 if you have the `floor' function. */
/* #undef HAVE_FLOOR */

/* Define to 1 if you have the `getopt' function. */
/* #undef HAVE_GETOPT */

/* Define as 0 or 1 according to the floating point format suported by the
   machine */
#define HAVE_IEEEFP 1

/* Define to 1 if the system has the type `int16'. */
#ifdef Q_OS_AIX
#define HAVE_INT16 1
#endif

/* Define to 1 if the system has the type `int32'. */
#ifdef Q_OS_AIX
#define HAVE_INT32 1
#endif

/* Define to 1 if the system has the type `int8'. */
#ifdef Q_OS_AIX
#define HAVE_INT8 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define to 1 if you have the `isascii' function. */
/* #undef HAVE_ISASCII */

/* Define to 1 if you have the `jbg_newlen' function. */
/* #undef HAVE_JBG_NEWLEN */

/* Define to 1 if you have the `lfind' function. */
/* #undef HAVE_LFIND */

/* Define to 1 if you have the `c' library (-lc). */
/* #undef HAVE_LIBC */

/* Define to 1 if you have the `m' library (-lm). */
/* #undef HAVE_LIBM */

/* Define to 1 if you have the <limits.h> header file. */
/* #undef HAVE_LIMITS_H */

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the `memmove' function. */
/* #undef HAVE_MEMMOVE */

/* Define to 1 if you have the <memory.h> header file. */
/* #undef HAVE_MEMORY_H */

/* Define to 1 if you have the `memset' function. */
/* #undef HAVE_MEMSET */

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the `pow' function. */
/* #undef HAVE_POW */

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Define to 1 if you have the <search.h> header file. */
#if !defined(Q_OS_WINCE) && !defined(Q_OS_SYMBIAN) && !defined(Q_OS_VXWORKS)
#define HAVE_SEARCH_H 1
#endif

/* Define to 1 if you have the `setmode' function. */
/* #undef HAVE_SETMODE */

/* Define to 1 if you have the `sqrt' function. */
/* #undef HAVE_SQRT */

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
/* #undef HAVE_STDLIB_H */

/* Define to 1 if you have the `strcasecmp' function. */
/* #undef HAVE_STRCASECMP */

/* Define to 1 if you have the `strchr' function. */
/* #undef HAVE_STRCHR */

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
/* #undef HAVE_STRRCHR */

/* Define to 1 if you have the `strstr' function. */
/* #undef HAVE_STRSTR */

/* Define to 1 if you have the `strtol' function. */
/* #undef HAVE_STRTOL */

/* Define to 1 if you have the `strtoul' function. */
/* #undef HAVE_STRTOUL */

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#if !defined(Q_OS_WIN)
#define HAVE_UNISTD_H 1
#endif

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Native cpu byte order: 1 if big-endian (Motorola) or 0 if little-endian
   (Intel) */
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
#define HOST_BIGENDIAN 1
#else
#define HOST_BIGENDIAN 0
#endif

/* Set the native cpu bit order (FILLORDER_LSB2MSB or FILLORDER_MSB2LSB) */
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* Support ISO JBIG compression (requires JBIG-KIT library) */
/* #undef JBIG_SUPPORT */

/* Support JPEG compression (requires IJG JPEG library) */
/* #undef JPEG_SUPPORT */

/* Support LogLuv high dynamic range encoding */
#define LOGLUV_SUPPORT 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
/* #undef LT_OBJDIR */

/* Support LZW algorithm */
#define LZW_SUPPORT 1

/* Support Microsoft Document Imaging format */
#define MDI_SUPPORT 1

/* Support NeXT 2-bit RLE algorithm */
#define NEXT_SUPPORT 1

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Support Old JPEG compresson (read-only) */
/* #undef OJPEG_SUPPORT */

/* Name of package */
/* #undef PACKAGE */

/* Define to the address where bug reports for this package should be sent. */
/* #undef PACKAGE_BUGREPORT */

/* Define to the full name of this package. */
/* #undef PACKAGE_NAME */

/* Define to the full name and version of this package. */
/* #undef PACKAGE_STRING */

/* Define to the one symbol short name of this package. */
/* #undef PACKAGE_TARNAME */

/* Define to the home page for this package. */
/* #undef PACKAGE_URL */

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.9.2"

/* Support Macintosh PackBits algorithm */
#define PACKBITS_SUPPORT 1

/* Support Pixar log-format algorithm (requires Zlib) */
#define PIXARLOG_SUPPORT 1

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#if (QT_POINTER_SIZE == 8) && !defined(Q_OS_WIN64)
#define SIZEOF_LONG 8
#else
#define SIZEOF_LONG 4
#endif

/* The size of `signed long', as computed by sizeof. */
/* #undef SIZEOF_SIGNED_LONG */

/* The size of `signed long long', as computed by sizeof. */
/* #undef SIZEOF_SIGNED_LONG_LONG */

/* The size of `unsigned long', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_LONG */

/* The size of `unsigned long long', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_LONG_LONG */

/* Define to 1 if you have the ANSI C header files. */
/* #undef STDC_HEADERS */

/* Support strip chopping (whether or not to convert single-strip uncompressed
   images to mutiple strips of specified size to reduce memory usage) */
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP

/* Default size of the strip in bytes (when strip chopping enabled) */
/* #undef STRIP_SIZE_DEFAULT */

/* Enable SubIFD tag (330) support */
#define SUBIFD_SUPPORT 1

/* Support ThunderScan 4-bit RLE algorithm */
#define THUNDER_SUPPORT 1

/* Signed 64-bit type formatter */
/* #undef TIFF_INT64_FORMAT */

/* Signed 64-bit type */
#define TIFF_INT64_T qint64

/* Unsigned 64-bit type formatter */
/* #undef TIFF_UINT64_FORMAT */

/* Unsigned 64-bit type */
#define TIFF_UINT64_T quint64

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
/* #undef TIME_WITH_SYS_TIME */

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Version number of package */
/* #undef VERSION */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
#define WORDS_BIGENDIAN 1
#endif

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Support Deflate compression */
#define ZIP_SUPPORT 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef Q_OS_SYMBIAN
#ifndef __cplusplus
#undef inline
#define inline
#endif
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

#ifdef Q_OS_WIN
#define TIF_PLATFORM_CONSOLE
#endif
