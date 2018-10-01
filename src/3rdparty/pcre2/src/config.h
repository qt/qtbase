#define HAVE_INTTYPES_H 1
#define HAVE_MEMMOVE 1
#define HAVE_LIMITS_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

#define LINK_SIZE 2
#define HEAP_LIMIT 20000000
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_DEPTH MATCH_LIMIT
#define MAX_NAME_COUNT 10000
#define MAX_NAME_SIZE 32
#define NEWLINE_DEFAULT 2
#define PARENS_NEST_LIMIT 250

#define SUPPORT_UNICODE

/*
    man 3 pcre2jit for a list of supported platforms;
    as PCRE2 10.22, stable JIT support is available for:
    - ARM 32-bit (v5, v7, and Thumb2)
    - ARM 64-bit
    - Intel x86 32-bit and 64-bit
    - MIPS 32-bit and 64-bit
    - Power PC 32-bit and 64-bit
    - SPARC 32-bit

    For non-x86 platforms we stick to the __GNUC__ compilers only.
*/
#if !defined(PCRE2_DISABLE_JIT) && (\
    /* ARM */ \
    (defined(__GNUC__) \
        && (defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM) || defined(__aarch64__))) \
    /* x86 32/64 */ \
    || defined(__i386) || defined(__i386__) || defined(_M_IX86) \
    || defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64) \
    /* MIPS */ \
    || (defined(__GNUC__) \
       && (defined(__mips) || defined(__mips__))) \
    /* PPC */ \
    || (defined(__GNUC__) \
       && (defined(__ppc__) || defined(__ppc) || defined(__powerpc__) \
          || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \
          || defined(_M_MPPC) || defined(_M_PPC))) \
    /* SPARC */ \
    || (defined(__GNUC__) \
       && (defined(__sparc__) && !defined(__sparc64__))) \
    )
#  define SUPPORT_JIT
#endif

