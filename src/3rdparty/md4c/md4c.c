/*
 * MD4C: Markdown parser for C
 * (http://github.com/mity/md4c)
 *
 * Copyright (c) 2016-2019 Martin Mitas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "md4c.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*****************************
 ***  Miscellaneous Stuff  ***
 *****************************/

#ifdef _MSC_VER
    /* MSVC does not understand "inline" when building as pure C (not C++).
     * However it understands "__inline" */
    #ifndef __cplusplus
        #define inline __inline
    #endif
#endif

#ifdef _T
    #undef _T
#endif
#if defined MD4C_USE_UTF16
    #define _T(x)           L##x
#else
    #define _T(x)           x
#endif

/* Misc. macros. */
#define SIZEOF_ARRAY(a)     (sizeof(a) / sizeof(a[0]))

#define STRINGIZE_(x)       #x
#define STRINGIZE(x)        STRINGIZE_(x)

#ifndef TRUE
    #define TRUE            1
    #define FALSE           0
#endif


/************************
 ***  Internal Types  ***
 ************************/

/* These are omnipresent so lets save some typing. */
#define CHAR    MD_CHAR
#define SZ      MD_SIZE
#define OFF     MD_OFFSET

typedef struct MD_MARK_tag MD_MARK;
typedef struct MD_BLOCK_tag MD_BLOCK;
typedef struct MD_CONTAINER_tag MD_CONTAINER;
typedef struct MD_REF_DEF_tag MD_REF_DEF;


/* During analyzes of inline marks, we need to manage some "mark chains",
 * of (yet unresolved) openers. This structure holds start/end of the chain.
 * The chain internals are then realized through MD_MARK::prev and ::next.
 */
typedef struct MD_MARKCHAIN_tag MD_MARKCHAIN;
struct MD_MARKCHAIN_tag {
    int head;   /* Index of first mark in the chain, or -1 if empty. */
    int tail;   /* Index of last mark in the chain, or -1 if empty. */
};

/* Context propagated through all the parsing. */
typedef struct MD_CTX_tag MD_CTX;
struct MD_CTX_tag {
    /* Immutable stuff (parameters of md_parse()). */
    const CHAR* text;
    SZ size;
    MD_PARSER parser;
    void* userdata;

    /* Helper temporary growing buffer. */
    CHAR* buffer;
    unsigned alloc_buffer;

    /* Reference definitions. */
    MD_REF_DEF* ref_defs;
    int n_ref_defs;
    int alloc_ref_defs;
    void** ref_def_hashtable;
    int ref_def_hashtable_size;

    /* Stack of inline/span markers.
     * This is only used for parsing a single block contents but by storing it
     * here we may reuse the stack for subsequent blocks; i.e. we have fewer
     * (re)allocations. */
    MD_MARK* marks;
    int n_marks;
    int alloc_marks;

#if defined MD4C_USE_UTF16
    char mark_char_map[128];
#else
    char mark_char_map[256];
#endif

    /* For resolving of inline spans. */
    MD_MARKCHAIN mark_chains[8];
#define PTR_CHAIN               ctx->mark_chains[0]
#define TABLECELLBOUNDARIES     ctx->mark_chains[1]
#define BACKTICK_OPENERS        ctx->mark_chains[2]
#define LOWERTHEN_OPENERS       ctx->mark_chains[3]
#define ASTERISK_OPENERS        ctx->mark_chains[4]
#define UNDERSCORE_OPENERS      ctx->mark_chains[5]
#define TILDE_OPENERS           ctx->mark_chains[6]
#define BRACKET_OPENERS         ctx->mark_chains[7]
#define OPENERS_CHAIN_FIRST     2
#define OPENERS_CHAIN_LAST      7

    int n_table_cell_boundaries;

    /* For resolving links. */
    int unresolved_link_head;
    int unresolved_link_tail;

    /* For block analysis.
     * Notes:
     *   -- It holds MD_BLOCK as well as MD_LINE structures. After each
     *      MD_BLOCK, its (multiple) MD_LINE(s) follow.
     *   -- For MD_BLOCK_HTML and MD_BLOCK_CODE, MD_VERBATIMLINE(s) are used
     *      instead of MD_LINE(s).
     */
    void* block_bytes;
    MD_BLOCK* current_block;
    int n_block_bytes;
    int alloc_block_bytes;

    /* For container block analysis. */
    MD_CONTAINER* containers;
    int n_containers;
    int alloc_containers;

    int last_line_has_list_loosening_effect;
    int last_list_item_starts_with_two_blank_lines;

    /* Minimal indentation to call the block "indented code block". */
    unsigned code_indent_offset;

    /* Contextual info for line analysis. */
    SZ code_fence_length;   /* For checking closing fence length. */
    int html_block_type;    /* For checking closing raw HTML condition. */
};

typedef enum MD_LINETYPE_tag MD_LINETYPE;
enum MD_LINETYPE_tag {
    MD_LINE_BLANK,
    MD_LINE_HR,
    MD_LINE_ATXHEADER,
    MD_LINE_SETEXTHEADER,
    MD_LINE_SETEXTUNDERLINE,
    MD_LINE_INDENTEDCODE,
    MD_LINE_FENCEDCODE,
    MD_LINE_HTML,
    MD_LINE_TEXT,
    MD_LINE_TABLE,
    MD_LINE_TABLEUNDERLINE
};

typedef struct MD_LINE_ANALYSIS_tag MD_LINE_ANALYSIS;
struct MD_LINE_ANALYSIS_tag {
    MD_LINETYPE type    : 16;
    unsigned data       : 16;
    OFF beg;
    OFF end;
    unsigned indent;        /* Indentation level. */
};

typedef struct MD_LINE_tag MD_LINE;
struct MD_LINE_tag {
    OFF beg;
    OFF end;
};

typedef struct MD_VERBATIMLINE_tag MD_VERBATIMLINE;
struct MD_VERBATIMLINE_tag {
    OFF beg;
    OFF end;
    OFF indent;
};


/*******************
 ***  Debugging  ***
 *******************/

#define MD_LOG(msg)                                                     \
    do {                                                                \
        if(ctx->parser.debug_log != NULL)                               \
            ctx->parser.debug_log((msg), ctx->userdata);                \
    } while(0)

#ifdef DEBUG
    #define MD_ASSERT(cond)                                             \
            do {                                                        \
                if(!(cond)) {                                           \
                    MD_LOG(__FILE__ ":" STRINGIZE(__LINE__) ": "        \
                           "Assertion '" STRINGIZE(cond) "' failed.");  \
                    exit(1);                                            \
                }                                                       \
            } while(0)

    #define MD_UNREACHABLE()        MD_ASSERT(1 == 0)
#else
    #ifdef __GNUC__
        #define MD_ASSERT(cond)     do { if(!(cond)) __builtin_unreachable(); } while(0)
        #define MD_UNREACHABLE()    do { __builtin_unreachable(); } while(0)
    #elif defined _MSC_VER  &&  _MSC_VER > 120
        #define MD_ASSERT(cond)     do { __assume(cond); } while(0)
        #define MD_UNREACHABLE()    do { __assume(0); } while(0)
    #else
        #define MD_ASSERT(cond)     do {} while(0)
        #define MD_UNREACHABLE()    do {} while(0)
    #endif
#endif


/*****************
 ***  Helpers  ***
 *****************/

/* Character accessors. */
#define CH(off)                 (ctx->text[(off)])
#define STR(off)                (ctx->text + (off))

/* Character classification.
 * Note we assume ASCII compatibility of code points < 128 here. */
#define ISIN_(ch, ch_min, ch_max)       ((ch_min) <= (unsigned)(ch) && (unsigned)(ch) <= (ch_max))
#define ISANYOF_(ch, palette)           (md_strchr((palette), (ch)) != NULL)
#define ISANYOF2_(ch, ch1, ch2)         ((ch) == (ch1) || (ch) == (ch2))
#define ISANYOF3_(ch, ch1, ch2, ch3)    ((ch) == (ch1) || (ch) == (ch2) || (ch) == (ch3))
#define ISASCII_(ch)                    ((unsigned)(ch) <= 127)
#define ISBLANK_(ch)                    (ISANYOF2_((ch), _T(' '), _T('\t')))
#define ISNEWLINE_(ch)                  (ISANYOF2_((ch), _T('\r'), _T('\n')))
#define ISWHITESPACE_(ch)               (ISBLANK_(ch) || ISANYOF2_((ch), _T('\v'), _T('\f')))
#define ISCNTRL_(ch)                    ((unsigned)(ch) <= 31 || (unsigned)(ch) == 127)
#define ISPUNCT_(ch)                    (ISIN_(ch, 33, 47) || ISIN_(ch, 58, 64) || ISIN_(ch, 91, 96) || ISIN_(ch, 123, 126))
#define ISUPPER_(ch)                    (ISIN_(ch, _T('A'), _T('Z')))
#define ISLOWER_(ch)                    (ISIN_(ch, _T('a'), _T('z')))
#define ISALPHA_(ch)                    (ISUPPER_(ch) || ISLOWER_(ch))
#define ISDIGIT_(ch)                    (ISIN_(ch, _T('0'), _T('9')))
#define ISXDIGIT_(ch)                   (ISDIGIT_(ch) || ISIN_(ch, _T('A'), _T('F')) || ISIN_(ch, _T('a'), _T('f')))
#define ISALNUM_(ch)                    (ISALPHA_(ch) || ISDIGIT_(ch))

#define ISANYOF(off, palette)           ISANYOF_(CH(off), (palette))
#define ISANYOF2(off, ch1, ch2)         ISANYOF2_(CH(off), (ch1), (ch2))
#define ISANYOF3(off, ch1, ch2, ch3)    ISANYOF3_(CH(off), (ch1), (ch2), (ch3))
#define ISASCII(off)                    ISASCII_(CH(off))
#define ISBLANK(off)                    ISBLANK_(CH(off))
#define ISNEWLINE(off)                  ISNEWLINE_(CH(off))
#define ISWHITESPACE(off)               ISWHITESPACE_(CH(off))
#define ISCNTRL(off)                    ISCNTRL_(CH(off))
#define ISPUNCT(off)                    ISPUNCT_(CH(off))
#define ISUPPER(off)                    ISUPPER_(CH(off))
#define ISLOWER(off)                    ISLOWER_(CH(off))
#define ISALPHA(off)                    ISALPHA_(CH(off))
#define ISDIGIT(off)                    ISDIGIT_(CH(off))
#define ISXDIGIT(off)                   ISXDIGIT_(CH(off))
#define ISALNUM(off)                    ISALNUM_(CH(off))
static inline const CHAR*
md_strchr(const CHAR* str, CHAR ch)
{
    OFF i;
    for(i = 0; str[i] != _T('\0'); i++) {
        if(ch == str[i])
            return (str + i);
    }
    return NULL;
}

/* Case insensitive check of string equality. */
static inline int
md_ascii_case_eq(const CHAR* s1, const CHAR* s2, SZ n)
{
    OFF i;
    for(i = 0; i < n; i++) {
        CHAR ch1 = s1[i];
        CHAR ch2 = s2[i];

        if(ISLOWER_(ch1))
            ch1 += ('A'-'a');
        if(ISLOWER_(ch2))
            ch2 += ('A'-'a');
        if(ch1 != ch2)
            return FALSE;
    }
    return TRUE;
}

static inline int
md_ascii_eq(const CHAR* s1, const CHAR* s2, SZ n)
{
    return memcmp(s1, s2, n * sizeof(CHAR)) == 0;
}

static int
md_text_with_null_replacement(MD_CTX* ctx, MD_TEXTTYPE type, const CHAR* str, SZ size)
{
    OFF off = 0;
    int ret = 0;

    while(1) {
        while(off < size  &&  str[off] != _T('\0'))
            off++;

        if(off > 0) {
            ret = ctx->parser.text(type, str, off, ctx->userdata);
            if(ret != 0)
                return ret;

            str += off;
            size -= off;
            off = 0;
        }

        if(off >= size)
            return 0;

        ret = ctx->parser.text(MD_TEXT_NULLCHAR, _T(""), 1, ctx->userdata);
        if(ret != 0)
            return ret;
        off++;
    }
}


#define MD_CHECK(func)                                                  \
    do {                                                                \
        ret = (func);                                                   \
        if(ret < 0)                                                     \
            goto abort;                                                 \
    } while(0)


#define MD_TEMP_BUFFER(sz)                                              \
    do {                                                                \
        if(sz > ctx->alloc_buffer) {                                    \
            CHAR* new_buffer;                                           \
            SZ new_size = ((sz) + (sz) / 2 + 128) & ~127;               \
                                                                        \
            new_buffer = realloc(ctx->buffer, new_size);                \
            if(new_buffer == NULL) {                                    \
                MD_LOG("realloc() failed.");                            \
                ret = -1;                                               \
                goto abort;                                             \
            }                                                           \
                                                                        \
            ctx->buffer = new_buffer;                                   \
            ctx->alloc_buffer = new_size;                               \
        }                                                               \
    } while(0)


#define MD_ENTER_BLOCK(type, arg)                                       \
    do {                                                                \
        ret = ctx->parser.enter_block((type), (arg), ctx->userdata);         \
        if(ret != 0) {                                                  \
            MD_LOG("Aborted from enter_block() callback.");             \
            goto abort;                                                 \
        }                                                               \
    } while(0)

#define MD_LEAVE_BLOCK(type, arg)                                       \
    do {                                                                \
        ret = ctx->parser.leave_block((type), (arg), ctx->userdata);         \
        if(ret != 0) {                                                  \
            MD_LOG("Aborted from leave_block() callback.");             \
            goto abort;                                                 \
        }                                                               \
    } while(0)

#define MD_ENTER_SPAN(type, arg)                                        \
    do {                                                                \
        ret = ctx->parser.enter_span((type), (arg), ctx->userdata);          \
        if(ret != 0) {                                                  \
            MD_LOG("Aborted from enter_span() callback.");              \
            goto abort;                                                 \
        }                                                               \
    } while(0)

#define MD_LEAVE_SPAN(type, arg)                                        \
    do {                                                                \
        ret = ctx->parser.leave_span((type), (arg), ctx->userdata);          \
        if(ret != 0) {                                                  \
            MD_LOG("Aborted from leave_span() callback.");              \
            goto abort;                                                 \
        }                                                               \
    } while(0)

#define MD_TEXT(type, str, size)                                        \
    do {                                                                \
        if(size > 0) {                                                  \
            ret = ctx->parser.text((type), (str), (size), ctx->userdata);    \
            if(ret != 0) {                                              \
                MD_LOG("Aborted from text() callback.");                \
                goto abort;                                             \
            }                                                           \
        }                                                               \
    } while(0)

#define MD_TEXT_INSECURE(type, str, size)                               \
    do {                                                                \
        if(size > 0) {                                                  \
            ret = md_text_with_null_replacement(ctx, type, str, size);  \
            if(ret != 0) {                                              \
                MD_LOG("Aborted from text() callback.");                \
                goto abort;                                             \
            }                                                           \
        }                                                               \
    } while(0)



/*************************
 ***  Unicode Support  ***
 *************************/

typedef struct MD_UNICODE_FOLD_INFO_tag MD_UNICODE_FOLD_INFO;
struct MD_UNICODE_FOLD_INFO_tag {
    int codepoints[3];
    int n_codepoints;
};


#if defined MD4C_USE_UTF16 || defined MD4C_USE_UTF8
    static int
    md_is_unicode_whitespace__(int codepoint)
    {
        /* The ASCII ones are the most frequently used ones, so lets check them first. */
        if(codepoint <= 0x7f)
            return ISWHITESPACE_(codepoint);

        /* Check for Unicode codepoints in Zs class above 127. */
        if(codepoint == 0x00a0 || codepoint == 0x1680)
            return TRUE;
        if(0x2000 <= codepoint && codepoint <= 0x200a)
            return TRUE;
        if(codepoint == 0x202f || codepoint == 0x205f || codepoint == 0x3000)
            return TRUE;

        return FALSE;
    }

    static int
    md_unicode_cmp__(const void* p_codepoint_a, const void* p_codepoint_b)
    {
        return (*(const int*)p_codepoint_a - *(const int*)p_codepoint_b);
    }

    static int
    md_is_unicode_punct__(int codepoint)
    {
        /* non-ASCII (above 127) Unicode punctuation codepoints (classes
         * Pc, Pd, Pe, Pf, Pi, Po, Ps).
         *
         * Warning: Keep the array sorted.
         */
        static const int punct_list[] = {
            0x00a1, 0x00a7, 0x00ab, 0x00b6, 0x00b7, 0x00bb, 0x00bf, 0x037e, 0x0387, 0x055a, 0x055b, 0x055c, 0x055d, 0x055e, 0x055f, 0x0589,
            0x058a, 0x05be, 0x05c0, 0x05c3, 0x05c6, 0x05f3, 0x05f4, 0x0609, 0x060a, 0x060c, 0x060d, 0x061b, 0x061e, 0x061f, 0x066a, 0x066b,
            0x066c, 0x066d, 0x06d4, 0x0700, 0x0701, 0x0702, 0x0703, 0x0704, 0x0705, 0x0706, 0x0707, 0x0708, 0x0709, 0x070a, 0x070b, 0x070c,
            0x070d, 0x07f7, 0x07f8, 0x07f9, 0x0830, 0x0831, 0x0832, 0x0833, 0x0834, 0x0835, 0x0836, 0x0837, 0x0838, 0x0839, 0x083a, 0x083b,
            0x083c, 0x083d, 0x083e, 0x085e, 0x0964, 0x0965, 0x0970, 0x0af0, 0x0df4, 0x0e4f, 0x0e5a, 0x0e5b, 0x0f04, 0x0f05, 0x0f06, 0x0f07,
            0x0f08, 0x0f09, 0x0f0a, 0x0f0b, 0x0f0c, 0x0f0d, 0x0f0e, 0x0f0f, 0x0f10, 0x0f11, 0x0f12, 0x0f14, 0x0f3a, 0x0f3b, 0x0f3c, 0x0f3d,
            0x0f85, 0x0fd0, 0x0fd1, 0x0fd2, 0x0fd3, 0x0fd4, 0x0fd9, 0x0fda, 0x104a, 0x104b, 0x104c, 0x104d, 0x104e, 0x104f, 0x10fb, 0x1360,
            0x1361, 0x1362, 0x1363, 0x1364, 0x1365, 0x1366, 0x1367, 0x1368, 0x1400, 0x166d, 0x166e, 0x169b, 0x169c, 0x16eb, 0x16ec, 0x16ed,
            0x1735, 0x1736, 0x17d4, 0x17d5, 0x17d6, 0x17d8, 0x17d9, 0x17da, 0x1800, 0x1801, 0x1802, 0x1803, 0x1804, 0x1805, 0x1806, 0x1807,
            0x1808, 0x1809, 0x180a, 0x1944, 0x1945, 0x1a1e, 0x1a1f, 0x1aa0, 0x1aa1, 0x1aa2, 0x1aa3, 0x1aa4, 0x1aa5, 0x1aa6, 0x1aa8, 0x1aa9,
            0x1aaa, 0x1aab, 0x1aac, 0x1aad, 0x1b5a, 0x1b5b, 0x1b5c, 0x1b5d, 0x1b5e, 0x1b5f, 0x1b60, 0x1bfc, 0x1bfd, 0x1bfe, 0x1bff, 0x1c3b,
            0x1c3c, 0x1c3d, 0x1c3e, 0x1c3f, 0x1c7e, 0x1c7f, 0x1cc0, 0x1cc1, 0x1cc2, 0x1cc3, 0x1cc4, 0x1cc5, 0x1cc6, 0x1cc7, 0x1cd3, 0x2010,
            0x2011, 0x2012, 0x2013, 0x2014, 0x2015, 0x2016, 0x2017, 0x2018, 0x2019, 0x201a, 0x201b, 0x201c, 0x201d, 0x201e, 0x201f, 0x2020,
            0x2021, 0x2022, 0x2023, 0x2024, 0x2025, 0x2026, 0x2027, 0x2030, 0x2031, 0x2032, 0x2033, 0x2034, 0x2035, 0x2036, 0x2037, 0x2038,
            0x2039, 0x203a, 0x203b, 0x203c, 0x203d, 0x203e, 0x203f, 0x2040, 0x2041, 0x2042, 0x2043, 0x2045, 0x2046, 0x2047, 0x2048, 0x2049,
            0x204a, 0x204b, 0x204c, 0x204d, 0x204e, 0x204f, 0x2050, 0x2051, 0x2053, 0x2054, 0x2055, 0x2056, 0x2057, 0x2058, 0x2059, 0x205a,
            0x205b, 0x205c, 0x205d, 0x205e, 0x207d, 0x207e, 0x208d, 0x208e, 0x2308, 0x2309, 0x230a, 0x230b, 0x2329, 0x232a, 0x2768, 0x2769,
            0x276a, 0x276b, 0x276c, 0x276d, 0x276e, 0x276f, 0x2770, 0x2771, 0x2772, 0x2773, 0x2774, 0x2775, 0x27c5, 0x27c6, 0x27e6, 0x27e7,
            0x27e8, 0x27e9, 0x27ea, 0x27eb, 0x27ec, 0x27ed, 0x27ee, 0x27ef, 0x2983, 0x2984, 0x2985, 0x2986, 0x2987, 0x2988, 0x2989, 0x298a,
            0x298b, 0x298c, 0x298d, 0x298e, 0x298f, 0x2990, 0x2991, 0x2992, 0x2993, 0x2994, 0x2995, 0x2996, 0x2997, 0x2998, 0x29d8, 0x29d9,
            0x29da, 0x29db, 0x29fc, 0x29fd, 0x2cf9, 0x2cfa, 0x2cfb, 0x2cfc, 0x2cfe, 0x2cff, 0x2d70, 0x2e00, 0x2e01, 0x2e02, 0x2e03, 0x2e04,
            0x2e05, 0x2e06, 0x2e07, 0x2e08, 0x2e09, 0x2e0a, 0x2e0b, 0x2e0c, 0x2e0d, 0x2e0e, 0x2e0f, 0x2e10, 0x2e11, 0x2e12, 0x2e13, 0x2e14,
            0x2e15, 0x2e16, 0x2e17, 0x2e18, 0x2e19, 0x2e1a, 0x2e1b, 0x2e1c, 0x2e1d, 0x2e1e, 0x2e1f, 0x2e20, 0x2e21, 0x2e22, 0x2e23, 0x2e24,
            0x2e25, 0x2e26, 0x2e27, 0x2e28, 0x2e29, 0x2e2a, 0x2e2b, 0x2e2c, 0x2e2d, 0x2e2e, 0x2e30, 0x2e31, 0x2e32, 0x2e33, 0x2e34, 0x2e35,
            0x2e36, 0x2e37, 0x2e38, 0x2e39, 0x2e3a, 0x2e3b, 0x2e3c, 0x2e3d, 0x2e3e, 0x2e3f, 0x2e40, 0x2e41, 0x2e42, 0x2e43, 0x2e44, 0x3001,
            0x3002, 0x3003, 0x3008, 0x3009, 0x300a, 0x300b, 0x300c, 0x300d, 0x300e, 0x300f, 0x3010, 0x3011, 0x3014, 0x3015, 0x3016, 0x3017,
            0x3018, 0x3019, 0x301a, 0x301b, 0x301c, 0x301d, 0x301e, 0x301f, 0x3030, 0x303d, 0x30a0, 0x30fb, 0xa4fe, 0xa4ff, 0xa60d, 0xa60e,
            0xa60f, 0xa673, 0xa67e, 0xa6f2, 0xa6f3, 0xa6f4, 0xa6f5, 0xa6f6, 0xa6f7, 0xa874, 0xa875, 0xa876, 0xa877, 0xa8ce, 0xa8cf, 0xa8f8,
            0xa8f9, 0xa8fa, 0xa8fc, 0xa92e, 0xa92f, 0xa95f, 0xa9c1, 0xa9c2, 0xa9c3, 0xa9c4, 0xa9c5, 0xa9c6, 0xa9c7, 0xa9c8, 0xa9c9, 0xa9ca,
            0xa9cb, 0xa9cc, 0xa9cd, 0xa9de, 0xa9df, 0xaa5c, 0xaa5d, 0xaa5e, 0xaa5f, 0xaade, 0xaadf, 0xaaf0, 0xaaf1, 0xabeb, 0xfd3e, 0xfd3f,
            0xfe10, 0xfe11, 0xfe12, 0xfe13, 0xfe14, 0xfe15, 0xfe16, 0xfe17, 0xfe18, 0xfe19, 0xfe30, 0xfe31, 0xfe32, 0xfe33, 0xfe34, 0xfe35,
            0xfe36, 0xfe37, 0xfe38, 0xfe39, 0xfe3a, 0xfe3b, 0xfe3c, 0xfe3d, 0xfe3e, 0xfe3f, 0xfe40, 0xfe41, 0xfe42, 0xfe43, 0xfe44, 0xfe45,
            0xfe46, 0xfe47, 0xfe48, 0xfe49, 0xfe4a, 0xfe4b, 0xfe4c, 0xfe4d, 0xfe4e, 0xfe4f, 0xfe50, 0xfe51, 0xfe52, 0xfe54, 0xfe55, 0xfe56,
            0xfe57, 0xfe58, 0xfe59, 0xfe5a, 0xfe5b, 0xfe5c, 0xfe5d, 0xfe5e, 0xfe5f, 0xfe60, 0xfe61, 0xfe63, 0xfe68, 0xfe6a, 0xfe6b, 0xff01,
            0xff02, 0xff03, 0xff05, 0xff06, 0xff07, 0xff08, 0xff09, 0xff0a, 0xff0c, 0xff0d, 0xff0e, 0xff0f, 0xff1a, 0xff1b, 0xff1f, 0xff20,
            0xff3b, 0xff3c, 0xff3d, 0xff3f, 0xff5b, 0xff5d, 0xff5f, 0xff60, 0xff61, 0xff62, 0xff63, 0xff64, 0xff65, 0x10100, 0x10101, 0x10102,
            0x1039f, 0x103d0, 0x1056f, 0x10857, 0x1091f, 0x1093f, 0x10a50, 0x10a51, 0x10a52, 0x10a53, 0x10a54, 0x10a55, 0x10a56, 0x10a57, 0x10a58, 0x10a7f,
            0x10af0, 0x10af1, 0x10af2, 0x10af3, 0x10af4, 0x10af5, 0x10af6, 0x10b39, 0x10b3a, 0x10b3b, 0x10b3c, 0x10b3d, 0x10b3e, 0x10b3f, 0x10b99, 0x10b9a,
            0x10b9b, 0x10b9c, 0x11047, 0x11048, 0x11049, 0x1104a, 0x1104b, 0x1104c, 0x1104d, 0x110bb, 0x110bc, 0x110be, 0x110bf, 0x110c0, 0x110c1, 0x11140,
            0x11141, 0x11142, 0x11143, 0x11174, 0x11175, 0x111c5, 0x111c6, 0x111c7, 0x111c8, 0x111c9, 0x111cd, 0x111db, 0x111dd, 0x111de, 0x111df, 0x11238,
            0x11239, 0x1123a, 0x1123b, 0x1123c, 0x1123d, 0x112a9, 0x1144b, 0x1144c, 0x1144d, 0x1144e, 0x1144f, 0x1145b, 0x1145d, 0x114c6, 0x115c1, 0x115c2,
            0x115c3, 0x115c4, 0x115c5, 0x115c6, 0x115c7, 0x115c8, 0x115c9, 0x115ca, 0x115cb, 0x115cc, 0x115cd, 0x115ce, 0x115cf, 0x115d0, 0x115d1, 0x115d2,
            0x115d3, 0x115d4, 0x115d5, 0x115d6, 0x115d7, 0x11641, 0x11642, 0x11643, 0x11660, 0x11661, 0x11662, 0x11663, 0x11664, 0x11665, 0x11666, 0x11667,
            0x11668, 0x11669, 0x1166a, 0x1166b, 0x1166c, 0x1173c, 0x1173d, 0x1173e, 0x11c41, 0x11c42, 0x11c43, 0x11c44, 0x11c45, 0x11c70, 0x11c71, 0x12470,
            0x12471, 0x12472, 0x12473, 0x12474, 0x16a6e, 0x16a6f, 0x16af5, 0x16b37, 0x16b38, 0x16b39, 0x16b3a, 0x16b3b, 0x16b44, 0x1bc9f, 0x1da87, 0x1da88,
            0x1da89, 0x1da8a, 0x1da8b, 0x1e95e, 0x1e95f
        };

        /* The ASCII ones are the most frequently used ones, so lets check them first. */
        if(codepoint <= 0x7f)
            return ISPUNCT_(codepoint);

        return (bsearch(&codepoint, punct_list, SIZEOF_ARRAY(punct_list), sizeof(int), md_unicode_cmp__) != NULL);
    }

    static void
    md_get_unicode_fold_info(int codepoint, MD_UNICODE_FOLD_INFO* info)
    {
        /* This maps single codepoint within a range to a single codepoint
         * within an offseted range. */
        static const struct {
            int min_codepoint;
            int max_codepoint;
            int offset;
        } range_map[] = {
            { 0x00c0, 0x00d6, 32 }, { 0x00d8, 0x00de, 32 }, { 0x0388, 0x038a, 37 }, { 0x0391, 0x03a1, 32 }, { 0x03a3, 0x03ab, 32 }, { 0x0400, 0x040f, 80 },
            { 0x0410, 0x042f, 32 }, { 0x0531, 0x0556, 48 }, { 0x1f08, 0x1f0f, -8 }, { 0x1f18, 0x1f1d, -8 }, { 0x1f28, 0x1f2f, -8 }, { 0x1f38, 0x1f3f, -8 },
            { 0x1f48, 0x1f4d, -8 }, { 0x1f68, 0x1f6f, -8 }, { 0x1fc8, 0x1fcb, -86 }, { 0x2160, 0x216f, 16 }, { 0x24b6, 0x24cf, 26 }, { 0xff21, 0xff3a, 32 },
            { 0x10400, 0x10425, 40 }
        };

        /* This maps single codepoint to another single codepoint. */
        static const struct {
            int src_codepoint;
            int dest_codepoint;
        } single_map[] = {
            { 0x00b5, 0x03bc }, { 0x0100, 0x0101 }, { 0x0102, 0x0103 }, { 0x0104, 0x0105 }, { 0x0106, 0x0107 }, { 0x0108, 0x0109 }, { 0x010a, 0x010b }, { 0x010c, 0x010d },
            { 0x010e, 0x010f }, { 0x0110, 0x0111 }, { 0x0112, 0x0113 }, { 0x0114, 0x0115 }, { 0x0116, 0x0117 }, { 0x0118, 0x0119 }, { 0x011a, 0x011b }, { 0x011c, 0x011d },
            { 0x011e, 0x011f }, { 0x0120, 0x0121 }, { 0x0122, 0x0123 }, { 0x0124, 0x0125 }, { 0x0126, 0x0127 }, { 0x0128, 0x0129 }, { 0x012a, 0x012b }, { 0x012c, 0x012d },
            { 0x012e, 0x012f }, { 0x0132, 0x0133 }, { 0x0134, 0x0135 }, { 0x0136, 0x0137 }, { 0x0139, 0x013a }, { 0x013b, 0x013c }, { 0x013d, 0x013e }, { 0x013f, 0x0140 },
            { 0x0141, 0x0142 }, { 0x0143, 0x0144 }, { 0x0145, 0x0146 }, { 0x0147, 0x0148 }, { 0x014a, 0x014b }, { 0x014c, 0x014d }, { 0x014e, 0x014f }, { 0x0150, 0x0151 },
            { 0x0152, 0x0153 }, { 0x0154, 0x0155 }, { 0x0156, 0x0157 }, { 0x0158, 0x0159 }, { 0x015a, 0x015b }, { 0x015c, 0x015d }, { 0x015e, 0x015f }, { 0x0160, 0x0161 },
            { 0x0162, 0x0163 }, { 0x0164, 0x0165 }, { 0x0166, 0x0167 }, { 0x0168, 0x0169 }, { 0x016a, 0x016b }, { 0x016c, 0x016d }, { 0x016e, 0x016f }, { 0x0170, 0x0171 },
            { 0x0172, 0x0173 }, { 0x0174, 0x0175 }, { 0x0176, 0x0177 }, { 0x0178, 0x00ff }, { 0x0179, 0x017a }, { 0x017b, 0x017c }, { 0x017d, 0x017e }, { 0x017f, 0x0073 },
            { 0x0181, 0x0253 }, { 0x0182, 0x0183 }, { 0x0184, 0x0185 }, { 0x0186, 0x0254 }, { 0x0187, 0x0188 }, { 0x0189, 0x0256 }, { 0x018a, 0x0257 }, { 0x018b, 0x018c },
            { 0x018e, 0x01dd }, { 0x018f, 0x0259 }, { 0x0190, 0x025b }, { 0x0191, 0x0192 }, { 0x0193, 0x0260 }, { 0x0194, 0x0263 }, { 0x0196, 0x0269 }, { 0x0197, 0x0268 },
            { 0x0198, 0x0199 }, { 0x019c, 0x026f }, { 0x019d, 0x0272 }, { 0x019f, 0x0275 }, { 0x01a0, 0x01a1 }, { 0x01a2, 0x01a3 }, { 0x01a4, 0x01a5 }, { 0x01a6, 0x0280 },
            { 0x01a7, 0x01a8 }, { 0x01a9, 0x0283 }, { 0x01ac, 0x01ad }, { 0x01ae, 0x0288 }, { 0x01af, 0x01b0 }, { 0x01b1, 0x028a }, { 0x01b2, 0x028b }, { 0x01b3, 0x01b4 },
            { 0x01b5, 0x01b6 }, { 0x01b7, 0x0292 }, { 0x01b8, 0x01b9 }, { 0x01bc, 0x01bd }, { 0x01c4, 0x01c6 }, { 0x01c5, 0x01c6 }, { 0x01c7, 0x01c9 }, { 0x01c8, 0x01c9 },
            { 0x01ca, 0x01cc }, { 0x01cb, 0x01cc }, { 0x01cd, 0x01ce }, { 0x01cf, 0x01d0 }, { 0x01d1, 0x01d2 }, { 0x01d3, 0x01d4 }, { 0x01d5, 0x01d6 }, { 0x01d7, 0x01d8 },
            { 0x01d9, 0x01da }, { 0x01db, 0x01dc }, { 0x01de, 0x01df }, { 0x01e0, 0x01e1 }, { 0x01e2, 0x01e3 }, { 0x01e4, 0x01e5 }, { 0x01e6, 0x01e7 }, { 0x01e8, 0x01e9 },
            { 0x01ea, 0x01eb }, { 0x01ec, 0x01ed }, { 0x01ee, 0x01ef }, { 0x01f1, 0x01f3 }, { 0x01f2, 0x01f3 }, { 0x01f4, 0x01f5 }, { 0x01f6, 0x0195 }, { 0x01f7, 0x01bf },
            { 0x01f8, 0x01f9 }, { 0x01fa, 0x01fb }, { 0x01fc, 0x01fd }, { 0x01fe, 0x01ff }, { 0x0200, 0x0201 }, { 0x0202, 0x0203 }, { 0x0204, 0x0205 }, { 0x0206, 0x0207 },
            { 0x0208, 0x0209 }, { 0x020a, 0x020b }, { 0x020c, 0x020d }, { 0x020e, 0x020f }, { 0x0210, 0x0211 }, { 0x0212, 0x0213 }, { 0x0214, 0x0215 }, { 0x0216, 0x0217 },
            { 0x0218, 0x0219 }, { 0x021a, 0x021b }, { 0x021c, 0x021d }, { 0x021e, 0x021f }, { 0x0220, 0x019e }, { 0x0222, 0x0223 }, { 0x0224, 0x0225 }, { 0x0226, 0x0227 },
            { 0x0228, 0x0229 }, { 0x022a, 0x022b }, { 0x022c, 0x022d }, { 0x022e, 0x022f }, { 0x0230, 0x0231 }, { 0x0232, 0x0233 }, { 0x0345, 0x03b9 }, { 0x0386, 0x03ac },
            { 0x038c, 0x03cc }, { 0x038e, 0x03cd }, { 0x038f, 0x03ce }, { 0x03c2, 0x03c3 }, { 0x03d0, 0x03b2 }, { 0x03d1, 0x03b8 }, { 0x03d5, 0x03c6 }, { 0x03d6, 0x03c0 },
            { 0x03d8, 0x03d9 }, { 0x03da, 0x03db }, { 0x03dc, 0x03dd }, { 0x03de, 0x03df }, { 0x03e0, 0x03e1 }, { 0x03e2, 0x03e3 }, { 0x03e4, 0x03e5 }, { 0x03e6, 0x03e7 },
            { 0x03e8, 0x03e9 }, { 0x03ea, 0x03eb }, { 0x03ec, 0x03ed }, { 0x03ee, 0x03ef }, { 0x03f0, 0x03ba }, { 0x03f1, 0x03c1 }, { 0x03f2, 0x03c3 }, { 0x03f4, 0x03b8 },
            { 0x03f5, 0x03b5 }, { 0x0460, 0x0461 }, { 0x0462, 0x0463 }, { 0x0464, 0x0465 }, { 0x0466, 0x0467 }, { 0x0468, 0x0469 }, { 0x046a, 0x046b }, { 0x046c, 0x046d },
            { 0x046e, 0x046f }, { 0x0470, 0x0471 }, { 0x0472, 0x0473 }, { 0x0474, 0x0475 }, { 0x0476, 0x0477 }, { 0x0478, 0x0479 }, { 0x047a, 0x047b }, { 0x047c, 0x047d },
            { 0x047e, 0x047f }, { 0x0480, 0x0481 }, { 0x048a, 0x048b }, { 0x048c, 0x048d }, { 0x048e, 0x048f }, { 0x0490, 0x0491 }, { 0x0492, 0x0493 }, { 0x0494, 0x0495 },
            { 0x0496, 0x0497 }, { 0x0498, 0x0499 }, { 0x049a, 0x049b }, { 0x049c, 0x049d }, { 0x049e, 0x049f }, { 0x04a0, 0x04a1 }, { 0x04a2, 0x04a3 }, { 0x04a4, 0x04a5 },
            { 0x04a6, 0x04a7 }, { 0x04a8, 0x04a9 }, { 0x04aa, 0x04ab }, { 0x04ac, 0x04ad }, { 0x04ae, 0x04af }, { 0x04b0, 0x04b1 }, { 0x04b2, 0x04b3 }, { 0x04b4, 0x04b5 },
            { 0x04b6, 0x04b7 }, { 0x04b8, 0x04b9 }, { 0x04ba, 0x04bb }, { 0x04bc, 0x04bd }, { 0x04be, 0x04bf }, { 0x04c1, 0x04c2 }, { 0x04c3, 0x04c4 }, { 0x04c5, 0x04c6 },
            { 0x04c7, 0x04c8 }, { 0x04c9, 0x04ca }, { 0x04cb, 0x04cc }, { 0x04cd, 0x04ce }, { 0x04d0, 0x04d1 }, { 0x04d2, 0x04d3 }, { 0x04d4, 0x04d5 }, { 0x04d6, 0x04d7 },
            { 0x04d8, 0x04d9 }, { 0x04da, 0x04db }, { 0x04dc, 0x04dd }, { 0x04de, 0x04df }, { 0x04e0, 0x04e1 }, { 0x04e2, 0x04e3 }, { 0x04e4, 0x04e5 }, { 0x04e6, 0x04e7 },
            { 0x04e8, 0x04e9 }, { 0x04ea, 0x04eb }, { 0x04ec, 0x04ed }, { 0x04ee, 0x04ef }, { 0x04f0, 0x04f1 }, { 0x04f2, 0x04f3 }, { 0x04f4, 0x04f5 }, { 0x04f8, 0x04f9 },
            { 0x0500, 0x0501 }, { 0x0502, 0x0503 }, { 0x0504, 0x0505 }, { 0x0506, 0x0507 }, { 0x0508, 0x0509 }, { 0x050a, 0x050b }, { 0x050c, 0x050d }, { 0x050e, 0x050f },
            { 0x1e00, 0x1e01 }, { 0x1e02, 0x1e03 }, { 0x1e04, 0x1e05 }, { 0x1e06, 0x1e07 }, { 0x1e08, 0x1e09 }, { 0x1e0a, 0x1e0b }, { 0x1e0c, 0x1e0d }, { 0x1e0e, 0x1e0f },
            { 0x1e10, 0x1e11 }, { 0x1e12, 0x1e13 }, { 0x1e14, 0x1e15 }, { 0x1e16, 0x1e17 }, { 0x1e18, 0x1e19 }, { 0x1e1a, 0x1e1b }, { 0x1e1c, 0x1e1d }, { 0x1e1e, 0x1e1f },
            { 0x1e20, 0x1e21 }, { 0x1e22, 0x1e23 }, { 0x1e24, 0x1e25 }, { 0x1e26, 0x1e27 }, { 0x1e28, 0x1e29 }, { 0x1e2a, 0x1e2b }, { 0x1e2c, 0x1e2d }, { 0x1e2e, 0x1e2f },
            { 0x1e30, 0x1e31 }, { 0x1e32, 0x1e33 }, { 0x1e34, 0x1e35 }, { 0x1e36, 0x1e37 }, { 0x1e38, 0x1e39 }, { 0x1e3a, 0x1e3b }, { 0x1e3c, 0x1e3d }, { 0x1e3e, 0x1e3f },
            { 0x1e40, 0x1e41 }, { 0x1e42, 0x1e43 }, { 0x1e44, 0x1e45 }, { 0x1e46, 0x1e47 }, { 0x1e48, 0x1e49 }, { 0x1e4a, 0x1e4b }, { 0x1e4c, 0x1e4d }, { 0x1e4e, 0x1e4f },
            { 0x1e50, 0x1e51 }, { 0x1e52, 0x1e53 }, { 0x1e54, 0x1e55 }, { 0x1e56, 0x1e57 }, { 0x1e58, 0x1e59 }, { 0x1e5a, 0x1e5b }, { 0x1e5c, 0x1e5d }, { 0x1e5e, 0x1e5f },
            { 0x1e60, 0x1e61 }, { 0x1e62, 0x1e63 }, { 0x1e64, 0x1e65 }, { 0x1e66, 0x1e67 }, { 0x1e68, 0x1e69 }, { 0x1e6a, 0x1e6b }, { 0x1e6c, 0x1e6d }, { 0x1e6e, 0x1e6f },
            { 0x1e70, 0x1e71 }, { 0x1e72, 0x1e73 }, { 0x1e74, 0x1e75 }, { 0x1e76, 0x1e77 }, { 0x1e78, 0x1e79 }, { 0x1e7a, 0x1e7b }, { 0x1e7c, 0x1e7d }, { 0x1e7e, 0x1e7f },
            { 0x1e80, 0x1e81 }, { 0x1e82, 0x1e83 }, { 0x1e84, 0x1e85 }, { 0x1e86, 0x1e87 }, { 0x1e88, 0x1e89 }, { 0x1e8a, 0x1e8b }, { 0x1e8c, 0x1e8d }, { 0x1e8e, 0x1e8f },
            { 0x1e90, 0x1e91 }, { 0x1e92, 0x1e93 }, { 0x1e94, 0x1e95 }, { 0x1e9b, 0x1e61 }, { 0x1ea0, 0x1ea1 }, { 0x1ea2, 0x1ea3 }, { 0x1ea4, 0x1ea5 }, { 0x1ea6, 0x1ea7 },
            { 0x1ea8, 0x1ea9 }, { 0x1eaa, 0x1eab }, { 0x1eac, 0x1ead }, { 0x1eae, 0x1eaf }, { 0x1eb0, 0x1eb1 }, { 0x1eb2, 0x1eb3 }, { 0x1eb4, 0x1eb5 }, { 0x1eb6, 0x1eb7 },
            { 0x1eb8, 0x1eb9 }, { 0x1eba, 0x1ebb }, { 0x1ebc, 0x1ebd }, { 0x1ebe, 0x1ebf }, { 0x1ec0, 0x1ec1 }, { 0x1ec2, 0x1ec3 }, { 0x1ec4, 0x1ec5 }, { 0x1ec6, 0x1ec7 },
            { 0x1ec8, 0x1ec9 }, { 0x1eca, 0x1ecb }, { 0x1ecc, 0x1ecd }, { 0x1ece, 0x1ecf }, { 0x1ed0, 0x1ed1 }, { 0x1ed2, 0x1ed3 }, { 0x1ed4, 0x1ed5 }, { 0x1ed6, 0x1ed7 },
            { 0x1ed8, 0x1ed9 }, { 0x1eda, 0x1edb }, { 0x1edc, 0x1edd }, { 0x1ede, 0x1edf }, { 0x1ee0, 0x1ee1 }, { 0x1ee2, 0x1ee3 }, { 0x1ee4, 0x1ee5 }, { 0x1ee6, 0x1ee7 },
            { 0x1ee8, 0x1ee9 }, { 0x1eea, 0x1eeb }, { 0x1eec, 0x1eed }, { 0x1eee, 0x1eef }, { 0x1ef0, 0x1ef1 }, { 0x1ef2, 0x1ef3 }, { 0x1ef4, 0x1ef5 }, { 0x1ef6, 0x1ef7 },
            { 0x1ef8, 0x1ef9 }, { 0x1f59, 0x1f51 }, { 0x1f5b, 0x1f53 }, { 0x1f5d, 0x1f55 }, { 0x1f5f, 0x1f57 }, { 0x1fb8, 0x1fb0 }, { 0x1fb9, 0x1fb1 }, { 0x1fba, 0x1f70 },
            { 0x1fbb, 0x1f71 }, { 0x1fbe, 0x03b9 }, { 0x1fd8, 0x1fd0 }, { 0x1fd9, 0x1fd1 }, { 0x1fda, 0x1f76 }, { 0x1fdb, 0x1f77 }, { 0x1fe8, 0x1fe0 }, { 0x1fe9, 0x1fe1 },
            { 0x1fea, 0x1f7a }, { 0x1feb, 0x1f7b }, { 0x1fec, 0x1fe5 }, { 0x1ff8, 0x1f78 }, { 0x1ff9, 0x1f79 }, { 0x1ffa, 0x1f7c }, { 0x1ffb, 0x1f7d }, { 0x2126, 0x03c9 },
            { 0x212a, 0x006b }, { 0x212b, 0x00e5 },
        };

        /* This maps single codepoint to two codepoints. */
        static const struct {
            int src_codepoint;
            int dest_codepoint0;
            int dest_codepoint1;
        } double_map[] = {
            { 0x00df, 0x0073, 0x0073 }, { 0x0130, 0x0069, 0x0307 }, { 0x0149, 0x02bc, 0x006e }, { 0x01f0, 0x006a, 0x030c }, { 0x0587, 0x0565, 0x0582 }, { 0x1e96, 0x0068, 0x0331 },
            { 0x1e97, 0x0074, 0x0308 }, { 0x1e98, 0x0077, 0x030a }, { 0x1e99, 0x0079, 0x030a }, { 0x1e9a, 0x0061, 0x02be }, { 0x1f50, 0x03c5, 0x0313 }, { 0x1f80, 0x1f00, 0x03b9 },
            { 0x1f81, 0x1f01, 0x03b9 }, { 0x1f82, 0x1f02, 0x03b9 }, { 0x1f83, 0x1f03, 0x03b9 }, { 0x1f84, 0x1f04, 0x03b9 }, { 0x1f85, 0x1f05, 0x03b9 }, { 0x1f86, 0x1f06, 0x03b9 },
            { 0x1f87, 0x1f07, 0x03b9 }, { 0x1f88, 0x1f00, 0x03b9 }, { 0x1f89, 0x1f01, 0x03b9 }, { 0x1f8a, 0x1f02, 0x03b9 }, { 0x1f8b, 0x1f03, 0x03b9 }, { 0x1f8c, 0x1f04, 0x03b9 },
            { 0x1f8d, 0x1f05, 0x03b9 }, { 0x1f8e, 0x1f06, 0x03b9 }, { 0x1f8f, 0x1f07, 0x03b9 }, { 0x1f90, 0x1f20, 0x03b9 }, { 0x1f91, 0x1f21, 0x03b9 }, { 0x1f92, 0x1f22, 0x03b9 },
            { 0x1f93, 0x1f23, 0x03b9 }, { 0x1f94, 0x1f24, 0x03b9 }, { 0x1f95, 0x1f25, 0x03b9 }, { 0x1f96, 0x1f26, 0x03b9 }, { 0x1f97, 0x1f27, 0x03b9 }, { 0x1f98, 0x1f20, 0x03b9 },
            { 0x1f99, 0x1f21, 0x03b9 }, { 0x1f9a, 0x1f22, 0x03b9 }, { 0x1f9b, 0x1f23, 0x03b9 }, { 0x1f9c, 0x1f24, 0x03b9 }, { 0x1f9d, 0x1f25, 0x03b9 }, { 0x1f9e, 0x1f26, 0x03b9 },
            { 0x1f9f, 0x1f27, 0x03b9 }, { 0x1fa0, 0x1f60, 0x03b9 }, { 0x1fa1, 0x1f61, 0x03b9 }, { 0x1fa2, 0x1f62, 0x03b9 }, { 0x1fa3, 0x1f63, 0x03b9 }, { 0x1fa4, 0x1f64, 0x03b9 },
            { 0x1fa5, 0x1f65, 0x03b9 }, { 0x1fa6, 0x1f66, 0x03b9 }, { 0x1fa7, 0x1f67, 0x03b9 }, { 0x1fa8, 0x1f60, 0x03b9 }, { 0x1fa9, 0x1f61, 0x03b9 }, { 0x1faa, 0x1f62, 0x03b9 },
            { 0x1fab, 0x1f63, 0x03b9 }, { 0x1fac, 0x1f64, 0x03b9 }, { 0x1fad, 0x1f65, 0x03b9 }, { 0x1fae, 0x1f66, 0x03b9 }, { 0x1faf, 0x1f67, 0x03b9 }, { 0x1fb2, 0x1f70, 0x03b9 },
            { 0x1fb3, 0x03b1, 0x03b9 }, { 0x1fb4, 0x03ac, 0x03b9 }, { 0x1fb6, 0x03b1, 0x0342 }, { 0x1fbc, 0x03b1, 0x03b9 }, { 0x1fc2, 0x1f74, 0x03b9 }, { 0x1fc3, 0x03b7, 0x03b9 },
            { 0x1fc4, 0x03ae, 0x03b9 }, { 0x1fc6, 0x03b7, 0x0342 }, { 0x1fcc, 0x03b7, 0x03b9 }, { 0x1fd6, 0x03b9, 0x0342 }, { 0x1fe4, 0x03c1, 0x0313 }, { 0x1fe6, 0x03c5, 0x0342 },
            { 0x1ff2, 0x1f7c, 0x03b9 }, { 0x1ff3, 0x03c9, 0x03b9 }, { 0x1ff4, 0x03ce, 0x03b9 }, { 0x1ff6, 0x03c9, 0x0342 }, { 0x1ffc, 0x03c9, 0x03b9 }, { 0xfb00, 0x0066, 0x0066 },
            { 0xfb01, 0x0066, 0x0069 }, { 0xfb02, 0x0066, 0x006c }, { 0xfb05, 0x0073, 0x0074 }, { 0xfb06, 0x0073, 0x0074 }, { 0xfb13, 0x0574, 0x0576 }, { 0xfb14, 0x0574, 0x0565 },
            { 0xfb15, 0x0574, 0x056b }, { 0xfb16, 0x057e, 0x0576 }, { 0xfb17, 0x0574, 0x056d }
        };

        /* This maps single codepoint to three codepoints. */
        static const struct {
            int src_codepoint;
            int dest_codepoint0;
            int dest_codepoint1;
            int dest_codepoint2;
        } triple_map[] = {
            { 0x0390, 0x03b9, 0x0308, 0x0301 }, { 0x03b0, 0x03c5, 0x0308, 0x0301 }, { 0x1f52, 0x03c5, 0x0313, 0x0300 }, { 0x1f54, 0x03c5, 0x0313, 0x0301 },
            { 0x1f56, 0x03c5, 0x0313, 0x0342 }, { 0x1fb7, 0x03b1, 0x0342, 0x03b9 }, { 0x1fc7, 0x03b7, 0x0342, 0x03b9 }, { 0x1fd2, 0x03b9, 0x0308, 0x0300 },
            { 0x1fd3, 0x03b9, 0x0308, 0x0301 }, { 0x1fd7, 0x03b9, 0x0308, 0x0342 }, { 0x1fe2, 0x03c5, 0x0308, 0x0300 }, { 0x1fe3, 0x03c5, 0x0308, 0x0301 },
            { 0x1fe7, 0x03c5, 0x0308, 0x0342 }, { 0x1ff7, 0x03c9, 0x0342, 0x03b9 }, { 0xfb03, 0x0066, 0x0066, 0x0069 }, { 0xfb04, 0x0066, 0x0066, 0x006c }
        };

        int i;

        /* Fast path for ASCII characters. */
        if(codepoint <= 0x7f) {
            info->codepoints[0] = codepoint;
            if(ISUPPER_(codepoint))
                info->codepoints[0] += 'a' - 'A';
            info->n_codepoints = 1;
            return;
        }

        for(i = 0; i < SIZEOF_ARRAY(range_map); i++) {
            if(range_map[i].min_codepoint <= codepoint && codepoint <= range_map[i].max_codepoint) {
                info->codepoints[0] = codepoint + range_map[i].offset;
                info->n_codepoints = 1;
                return;
            }
        }

        for(i = 0; i < SIZEOF_ARRAY(single_map); i++) {
            if(codepoint == single_map[i].src_codepoint) {
                info->codepoints[0] = single_map[i].dest_codepoint;
                info->n_codepoints = 1;
                return;
            }
        }

        for(i = 0; i < SIZEOF_ARRAY(double_map); i++) {
            if(codepoint == double_map[i].src_codepoint) {
                info->codepoints[0] = double_map[i].dest_codepoint0;
                info->codepoints[1] = double_map[i].dest_codepoint1;
                info->n_codepoints = 2;
                return;
            }
        }

        for(i = 0; i < SIZEOF_ARRAY(triple_map); i++) {
            if(codepoint == triple_map[i].src_codepoint) {
                info->codepoints[0] = triple_map[i].dest_codepoint0;
                info->codepoints[1] = triple_map[i].dest_codepoint1;
                info->codepoints[2] = triple_map[i].dest_codepoint2;
                info->n_codepoints = 3;
                return;
            }
        }

        info->codepoints[0] = codepoint;
        info->n_codepoints = 1;
    }
#endif


#if defined MD4C_USE_UTF16
    #define IS_UTF16_SURROGATE_HI(word)     (((WORD)(word) & 0xfc00) == 0xd800)
    #define IS_UTF16_SURROGATE_LO(word)     (((WORD)(word) & 0xfc00) == 0xdc00)
    #define UTF16_DECODE_SURROGATE(hi, lo)  (0x10000 + ((((unsigned)(hi) & 0x3ff) << 10) | (((unsigned)(lo) & 0x3ff) << 0)))

    static int
    md_decode_utf16le__(const CHAR* str, SZ str_size, SZ* p_size)
    {
        if(IS_UTF16_SURROGATE_HI(str[0])) {
            if(1 < str_size && IS_UTF16_SURROGATE_LO(str[1])) {
                if(p_size != NULL)
                    *p_size = 2;
                return UTF16_DECODE_SURROGATE(str[0], str[1]);
            }
        }

        if(p_size != NULL)
            *p_size = 1;
        return str[0];
    }

    static int
    md_decode_utf16le_before__(MD_CTX* ctx, OFF off)
    {
        if(off > 2 && IS_UTF16_SURROGATE_HI(CH(off-2)) && IS_UTF16_SURROGATE_LO(CH(off-1)))
            return UTF16_DECODE_SURROGATE(CH(off-2), CH(off-1));

        return CH(off);
    }

    /* No whitespace uses surrogates, so no decoding needed here. */
    #define ISUNICODEWHITESPACE_(codepoint) md_is_unicode_whitespace__(codepoint)
    #define ISUNICODEWHITESPACE(off)        md_is_unicode_whitespace__(CH(off))
    #define ISUNICODEWHITESPACEBEFORE(off)  md_is_unicode_whitespace__(CH((off)-1))

    #define ISUNICODEPUNCT(off)             md_is_unicode_punct__(md_decode_utf16le__(STR(off), ctx->size - (off), NULL))
    #define ISUNICODEPUNCTBEFORE(off)       md_is_unicode_punct__(md_decode_utf16le_before__(ctx, off))

    static inline int
    md_decode_unicode(const CHAR* str, OFF off, SZ str_size, SZ* p_char_size)
    {
        return md_decode_utf16le__(str+off, str_size-off, p_char_size);
    }
#elif defined MD4C_USE_UTF8
    #define IS_UTF8_LEAD1(byte)     ((unsigned char)(byte) <= 0x7f)
    #define IS_UTF8_LEAD2(byte)     (((unsigned char)(byte) & 0xe0) == 0xc0)
    #define IS_UTF8_LEAD3(byte)     (((unsigned char)(byte) & 0xf0) == 0xe0)
    #define IS_UTF8_LEAD4(byte)     (((unsigned char)(byte) & 0xf8) == 0xf0)
    #define IS_UTF8_TAIL(byte)      (((unsigned char)(byte) & 0xc0) == 0x80)

    static int
    md_decode_utf8__(const CHAR* str, SZ str_size, SZ* p_size)
    {
        if(!IS_UTF8_LEAD1(str[0])) {
            if(IS_UTF8_LEAD2(str[0])) {
                if(1 < str_size && IS_UTF8_TAIL(str[1])) {
                    if(p_size != NULL)
                        *p_size = 2;

                    return (((unsigned int)str[0] & 0x1f) << 6) |
                           (((unsigned int)str[1] & 0x3f) << 0);
                }
            } else if(IS_UTF8_LEAD3(str[0])) {
                if(2 < str_size && IS_UTF8_TAIL(str[1]) && IS_UTF8_TAIL(str[2])) {
                    if(p_size != NULL)
                        *p_size = 3;

                    return (((unsigned int)str[0] & 0x0f) << 12) |
                           (((unsigned int)str[1] & 0x3f) << 6) |
                           (((unsigned int)str[2] & 0x3f) << 0);
                }
            } else if(IS_UTF8_LEAD4(str[0])) {
                if(3 < str_size && IS_UTF8_TAIL(str[1]) && IS_UTF8_TAIL(str[2]) && IS_UTF8_TAIL(str[3])) {
                    if(p_size != NULL)
                        *p_size = 4;

                    return (((unsigned int)str[0] & 0x07) << 18) |
                           (((unsigned int)str[1] & 0x3f) << 12) |
                           (((unsigned int)str[2] & 0x3f) << 6) |
                           (((unsigned int)str[3] & 0x3f) << 0);
                }
            }
        }

        if(p_size != NULL)
            *p_size = 1;
        return str[0];
    }

    static int
    md_decode_utf8_before__(MD_CTX* ctx, OFF off)
    {
        if(!IS_UTF8_LEAD1(CH(off-1))) {
            if(off > 1 && IS_UTF8_LEAD2(CH(off-2)) && IS_UTF8_TAIL(CH(off-1)))
                return (((unsigned int)CH(off-2) & 0x1f) << 6) |
                       (((unsigned int)CH(off-1) & 0x3f) << 0);

            if(off > 2 && IS_UTF8_LEAD3(CH(off-3)) && IS_UTF8_TAIL(CH(off-2)) && IS_UTF8_TAIL(CH(off-1)))
                return (((unsigned int)CH(off-3) & 0x0f) << 12) |
                       (((unsigned int)CH(off-2) & 0x3f) << 6) |
                       (((unsigned int)CH(off-1) & 0x3f) << 0);

            if(off > 3 && IS_UTF8_LEAD4(CH(off-4)) && IS_UTF8_TAIL(CH(off-3)) && IS_UTF8_TAIL(CH(off-2)) && IS_UTF8_TAIL(CH(off-1)))
                return (((unsigned int)CH(off-4) & 0x07) << 18) |
                       (((unsigned int)CH(off-3) & 0x3f) << 12) |
                       (((unsigned int)CH(off-2) & 0x3f) << 6) |
                       (((unsigned int)CH(off-1) & 0x3f) << 0);
        }

        return CH(off-1);
    }

    #define ISUNICODEWHITESPACE_(codepoint) md_is_unicode_whitespace__(codepoint)
    #define ISUNICODEWHITESPACE(off)        md_is_unicode_whitespace__(md_decode_utf8__(STR(off), ctx->size - (off), NULL))
    #define ISUNICODEWHITESPACEBEFORE(off)  md_is_unicode_whitespace__(md_decode_utf8_before__(ctx, off))

    #define ISUNICODEPUNCT(off)             md_is_unicode_punct__(md_decode_utf8__(STR(off), ctx->size - (off), NULL))
    #define ISUNICODEPUNCTBEFORE(off)       md_is_unicode_punct__(md_decode_utf8_before__(ctx, off))

    static inline int
    md_decode_unicode(const CHAR* str, OFF off, SZ str_size, SZ* p_char_size)
    {
        return md_decode_utf8__(str+off, str_size-off, p_char_size);
    }
#else
    #define ISUNICODEWHITESPACE_(codepoint) ISWHITESPACE_(codepoint)
    #define ISUNICODEWHITESPACE(off)        ISWHITESPACE(off)
    #define ISUNICODEWHITESPACEBEFORE(off)  ISWHITESPACE((off)-1)

    #define ISUNICODEPUNCT(off)             ISPUNCT(off)
    #define ISUNICODEPUNCTBEFORE(off)       ISPUNCT((off)-1)

    static inline void
    md_get_unicode_fold_info(int codepoint, MD_UNICODE_FOLD_INFO* info)
    {
        info->codepoints[0] = codepoint;
        if(ISUPPER_(codepoint))
            info->codepoints[0] += 'a' - 'A';
        info->n_codepoints = 1;
    }

    static inline int
    md_decode_unicode(const CHAR* str, OFF off, SZ str_size, SZ* p_size)
    {
        *p_size = 1;
        return str[off];
    }
#endif


/*************************************
 ***  Helper string manipulations  ***
 *************************************/

/* Fill buffer with copy of the string between 'beg' and 'end' but replace any
 * line breaks with given replacement character.
 *
 * NOTE: Caller is responsible to make sure the buffer is large enough.
 * (Given the output is always shorter then input, (end - beg) is good idea
 * what the caller should allocate.)
 */
static void
md_merge_lines(MD_CTX* ctx, OFF beg, OFF end, const MD_LINE* lines, int n_lines,
               CHAR line_break_replacement_char, CHAR* buffer, SZ* p_size)
{
    CHAR* ptr = buffer;
    int line_index = 0;
    OFF off = beg;

    while(1) {
        const MD_LINE* line = &lines[line_index];
        OFF line_end = line->end;
        if(end < line_end)
            line_end = end;

        while(off < line_end) {
            *ptr = CH(off);
            ptr++;
            off++;
        }

        if(off >= end) {
            *p_size = ptr - buffer;
            return;
        }

        *ptr = line_break_replacement_char;
        ptr++;

        line_index++;
        off = lines[line_index].beg;
    }
}

/* Wrapper of md_merge_lines() which allocates new buffer for the output string.
 */
static int
md_merge_lines_alloc(MD_CTX* ctx, OFF beg, OFF end, const MD_LINE* lines, int n_lines,
                    CHAR line_break_replacement_char, CHAR** p_str, SZ* p_size)
{
    CHAR* buffer;

    buffer = (CHAR*) malloc(sizeof(CHAR) * (end - beg));
    if(buffer == NULL) {
        MD_LOG("malloc() failed.");
        return -1;
    }

    md_merge_lines(ctx, beg, end, lines, n_lines,
                line_break_replacement_char, buffer, p_size);

    *p_str = buffer;
    return 0;
}

static OFF
md_skip_unicode_whitespace(const CHAR* label, OFF off, SZ size)
{
    SZ char_size;
    int codepoint;

    while(off < size) {
        codepoint = md_decode_unicode(label, off, size, &char_size);
        if(!ISUNICODEWHITESPACE_(codepoint)  &&  !ISNEWLINE_(label[off]))
            break;
        off += char_size;
    }

    return off;
}


/******************************
 ***  Recognizing raw HTML  ***
 ******************************/

/* md_is_html_tag() may be called when processing inlines (inline raw HTML)
 * or when breaking document to blocks (checking for start of HTML block type 7).
 *
 * When breaking document to blocks, we do not yet know line boundaries, but
 * in that case the whole tag has to live on a single line. We distinguish this
 * by n_lines == 0.
 */
static int
md_is_html_tag(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg, OFF max_end, OFF* p_end)
{
    int attr_state;
    OFF off = beg;
    OFF line_end = (n_lines > 0) ? lines[0].end : ctx->size;
    int i = 0;

    MD_ASSERT(CH(beg) == _T('<'));

    if(off + 1 >= line_end)
        return FALSE;
    off++;

    /* For parsing attributes, we need a little state automaton below.
     * State -1: no attributes are allowed.
     * State 0: attribute could follow after some whitespace.
     * State 1: after a whitespace (attribute name may follow).
     * State 2: after attribute name ('=' MAY follow).
     * State 3: after '=' (value specification MUST follow).
     * State 41: in middle of unquoted attribute value.
     * State 42: in middle of single-quoted attribute value.
     * State 43: in middle of double-quoted attribute value.
     */
    attr_state = 0;

    if(CH(off) == _T('/')) {
        /* Closer tag "</ ... >". No attributes may be present. */
        attr_state = -1;
        off++;
    }

    /* Tag name */
    if(off >= line_end  ||  !ISALPHA(off))
        return FALSE;
    off++;
    while(off < line_end  &&  (ISALNUM(off)  ||  CH(off) == _T('-')))
        off++;

    /* (Optional) attributes (if not closer), (optional) '/' (if not closer)
     * and final '>'. */
    while(1) {
        while(off < line_end  &&  !ISNEWLINE(off)) {
            if(attr_state > 40) {
                if(attr_state == 41 && (ISBLANK(off) || ISANYOF(off, _T("\"'=<>`")))) {
                    attr_state = 0;
                    off--;  /* Put the char back for re-inspection in the new state. */
                } else if(attr_state == 42 && CH(off) == _T('\'')) {
                    attr_state = 0;
                } else if(attr_state == 43 && CH(off) == _T('"')) {
                    attr_state = 0;
                }
                off++;
            } else if(ISWHITESPACE(off)) {
                if(attr_state == 0)
                    attr_state = 1;
                off++;
            } else if(attr_state <= 2 && CH(off) == _T('>')) {
                /* End. */
                goto done;
            } else if(attr_state <= 2 && CH(off) == _T('/') && off+1 < line_end && CH(off+1) == _T('>')) {
                /* End with digraph '/>' */
                off++;
                goto done;
            } else if((attr_state == 1 || attr_state == 2) && (ISALPHA(off) || CH(off) == _T('_') || CH(off) == _T(':'))) {
                off++;
                /* Attribute name */
                while(off < line_end && (ISALNUM(off) || ISANYOF(off, _T("_.:-"))))
                    off++;
                attr_state = 2;
            } else if(attr_state == 2 && CH(off) == _T('=')) {
                /* Attribute assignment sign */
                off++;
                attr_state = 3;
            } else if(attr_state == 3) {
                /* Expecting start of attribute value. */
                if(CH(off) == _T('"'))
                    attr_state = 43;
                else if(CH(off) == _T('\''))
                    attr_state = 42;
                else if(!ISANYOF(off, _T("\"'=<>`"))  &&  !ISNEWLINE(off))
                    attr_state = 41;
                else
                    return FALSE;
                off++;
            } else {
                /* Anything unexpected. */
                return FALSE;
            }
        }

        /* We have to be on a single line. See definition of start condition
         * of HTML block, type 7. */
        if(n_lines == 0)
            return FALSE;

        i++;
        if(i >= n_lines)
            return FALSE;

        off = lines[i].beg;
        line_end = lines[i].end;

        if(attr_state == 0  ||  attr_state == 41)
            attr_state = 1;

        if(off >= max_end)
            return FALSE;
    }

done:
    if(off >= max_end)
        return FALSE;

    *p_end = off+1;
    return TRUE;
}

static int
md_is_html_comment(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg, OFF max_end, OFF* p_end)
{
    OFF off = beg;
    int i = 0;

    MD_ASSERT(CH(beg) == _T('<'));

    if(off + 4 >= lines[0].end)
        return FALSE;
    if(CH(off+1) != _T('!')  ||  CH(off+2) != _T('-')  ||  CH(off+3) != _T('-'))
        return FALSE;
    off += 4;

    /* ">" and "->" must follow the opening. */
    if(off < lines[0].end  &&  CH(off) == _T('>'))
        return FALSE;
    if(off+1 < lines[0].end  &&  CH(off) == _T('-')  &&  CH(off+1) == _T('>'))
        return FALSE;

    while(1) {
        while(off + 2 < lines[i].end) {
            if(CH(off) == _T('-')  &&  CH(off+1) == _T('-')) {
                if(CH(off+2) == _T('>')) {
                    /* Success. */
                    off += 2;
                    goto done;
                } else {
                    /* "--" is prohibited inside the comment. */
                    return FALSE;
                }
            }

            off++;
        }

        i++;
        if(i >= n_lines)
            return FALSE;

        off = lines[i].beg;

        if(off >= max_end)
            return FALSE;
    }

done:
    if(off >= max_end)
        return FALSE;

    *p_end = off+1;
    return TRUE;
}

static int
md_is_html_processing_instruction(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg, OFF max_end, OFF* p_end)
{
    OFF off = beg;
    int i = 0;

    MD_ASSERT(CH(beg) == _T('<'));

    if(off + 2 >= lines[0].end)
        return FALSE;
    if(CH(off+1) != _T('?'))
        return FALSE;
    off += 2;

    while(1) {
        while(off + 1 < lines[i].end) {
            if(CH(off) == _T('?')  &&  CH(off+1) == _T('>')) {
                /* Success. */
                off++;
                goto done;
            }

            off++;
        }

        i++;
        if(i >= n_lines)
            return FALSE;

        off = lines[i].beg;
        if(off >= max_end)
            return FALSE;
    }

done:
    if(off >= max_end)
        return FALSE;

    *p_end = off+1;
    return TRUE;
}

static int
md_is_html_declaration(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg, OFF max_end, OFF* p_end)
{
    OFF off = beg;
    int i = 0;

    MD_ASSERT(CH(beg) == _T('<'));

    if(off + 2 >= lines[0].end)
        return FALSE;
    if(CH(off+1) != _T('!'))
        return FALSE;
    off += 2;

    /* Declaration name. */
    if(off >= lines[0].end  ||  !ISALPHA(off))
        return FALSE;
    off++;
    while(off < lines[0].end  &&  ISALPHA(off))
        off++;
    if(off < lines[0].end  &&  !ISWHITESPACE(off))
        return FALSE;

    while(1) {
        while(off < lines[i].end) {
            if(CH(off) == _T('>')) {
                /* Success. */
                goto done;
            }

            off++;
        }

        i++;
        if(i >= n_lines)
            return FALSE;

        off = lines[i].beg;
        if(off >= max_end)
            return FALSE;
    }

done:
    if(off >= max_end)
        return FALSE;

    *p_end = off+1;
    return TRUE;
}

static int
md_is_html_cdata(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg, OFF max_end, OFF* p_end)
{
    static const CHAR open_str[] = _T("<![CDATA[");
    static const SZ open_size = SIZEOF_ARRAY(open_str) - 1;

    OFF off = beg;
    int i = 0;

    if(off + open_size >= lines[0].end)
        return FALSE;
    if(memcmp(STR(off), open_str, open_size) != 0)
        return FALSE;
    off += open_size;

    while(1) {
        while(off + 2 < lines[i].end) {
            if(CH(off) == _T(']')  &&  CH(off+1) == _T(']')  &&  CH(off+2) == _T('>')) {
                /* Success. */
                off += 2;
                goto done;
            }

            off++;
        }

        i++;
        if(i >= n_lines)
            return FALSE;

        off = lines[i].beg;
        if(off >= max_end)
            return FALSE;
    }

done:
    if(off >= max_end)
        return FALSE;

    *p_end = off+1;
    return TRUE;
}

static int
md_is_html_any(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg, OFF max_end, OFF* p_end)
{
    if(md_is_html_tag(ctx, lines, n_lines, beg, max_end, p_end) == TRUE)
        return TRUE;
    if(md_is_html_comment(ctx, lines, n_lines, beg, max_end, p_end) == TRUE)
        return TRUE;
    if(md_is_html_processing_instruction(ctx, lines, n_lines, beg, max_end, p_end) == TRUE)
        return TRUE;
    if(md_is_html_declaration(ctx, lines, n_lines, beg, max_end, p_end) == TRUE)
        return TRUE;
    if(md_is_html_cdata(ctx, lines, n_lines, beg, max_end, p_end) == TRUE)
        return TRUE;

    return FALSE;
}


/****************************
 ***  Recognizing Entity  ***
 ****************************/

static int
md_is_hex_entity_contents(MD_CTX* ctx, const CHAR* text, OFF beg, OFF max_end, OFF* p_end)
{
    OFF off = beg;

    while(off < max_end  &&  ISXDIGIT_(text[off])  &&  off - beg <= 8)
        off++;

    if(1 <= off - beg  &&  off - beg <= 8) {
        *p_end = off;
        return TRUE;
    } else {
        return FALSE;
    }
}

static int
md_is_dec_entity_contents(MD_CTX* ctx, const CHAR* text, OFF beg, OFF max_end, OFF* p_end)
{
    OFF off = beg;

    while(off < max_end  &&  ISDIGIT_(text[off])  &&  off - beg <= 8)
        off++;

    if(1 <= off - beg  &&  off - beg <= 8) {
        *p_end = off;
        return TRUE;
    } else {
        return FALSE;
    }
}

static int
md_is_named_entity_contents(MD_CTX* ctx, const CHAR* text, OFF beg, OFF max_end, OFF* p_end)
{
    OFF off = beg;

    if(off < max_end  &&  ISALPHA_(text[off]))
        off++;
    else
        return FALSE;

    while(off < max_end  &&  ISALNUM_(text[off])  &&  off - beg <= 48)
        off++;

    if(2 <= off - beg  &&  off - beg <= 48) {
        *p_end = off;
        return TRUE;
    } else {
        return FALSE;
    }
}

static int
md_is_entity_str(MD_CTX* ctx, const CHAR* text, OFF beg, OFF max_end, OFF* p_end)
{
    int is_contents;
    OFF off = beg;

    MD_ASSERT(text[off] == _T('&'));
    off++;

    if(off+2 < max_end  &&  text[off] == _T('#')  &&  (text[off+1] == _T('x') || text[off+1] == _T('X')))
        is_contents = md_is_hex_entity_contents(ctx, text, off+2, max_end, &off);
    else if(off+1 < max_end  &&  text[off] == _T('#'))
        is_contents = md_is_dec_entity_contents(ctx, text, off+1, max_end, &off);
    else
        is_contents = md_is_named_entity_contents(ctx, text, off, max_end, &off);

    if(is_contents  &&  off < max_end  &&  text[off] == _T(';')) {
        *p_end = off+1;
        return TRUE;
    } else {
        return FALSE;
    }
}

static inline int
md_is_entity(MD_CTX* ctx, OFF beg, OFF max_end, OFF* p_end)
{
    return md_is_entity_str(ctx, ctx->text, beg, max_end, p_end);
}


/******************************
 ***  Attribute Management  ***
 ******************************/

typedef struct MD_ATTRIBUTE_BUILD_tag MD_ATTRIBUTE_BUILD;
struct MD_ATTRIBUTE_BUILD_tag {
    CHAR* text;
    MD_TEXTTYPE* substr_types;
    OFF* substr_offsets;
    int substr_count;
    int substr_alloc;
    MD_TEXTTYPE trivial_types[1];
    OFF trivial_offsets[2];
};


#define MD_BUILD_ATTR_NO_ESCAPES    0x0001

static int
md_build_attr_append_substr(MD_CTX* ctx, MD_ATTRIBUTE_BUILD* build,
                            MD_TEXTTYPE type, OFF off)
{
    if(build->substr_count >= build->substr_alloc) {
        MD_TEXTTYPE* new_substr_types;
        OFF* new_substr_offsets;

        build->substr_alloc = (build->substr_alloc == 0 ? 8 : build->substr_alloc * 2);

        new_substr_types = (MD_TEXTTYPE*) realloc(build->substr_types,
                                    build->substr_alloc * sizeof(MD_TEXTTYPE));
        if(new_substr_types == NULL) {
            MD_LOG("realloc() failed.");
            return -1;
        }
        /* Note +1 to reserve space for final offset (== raw_size). */
        new_substr_offsets = (OFF*) realloc(build->substr_offsets,
                                    (build->substr_alloc+1) * sizeof(OFF));
        if(new_substr_offsets == NULL) {
            MD_LOG("realloc() failed.");
            free(new_substr_types);
            return -1;
        }

        build->substr_types = new_substr_types;
        build->substr_offsets = new_substr_offsets;
    }

    build->substr_types[build->substr_count] = type;
    build->substr_offsets[build->substr_count] = off;
    build->substr_count++;
    return 0;
}

static void
md_free_attribute(MD_CTX* ctx, MD_ATTRIBUTE_BUILD* build)
{
    if(build->substr_alloc > 0) {
        free(build->text);
        free(build->substr_types);
        free(build->substr_offsets);
    }
}

static int
md_build_attribute(MD_CTX* ctx, const CHAR* raw_text, SZ raw_size,
                   unsigned flags, MD_ATTRIBUTE* attr, MD_ATTRIBUTE_BUILD* build)
{
    OFF raw_off, off;
    int is_trivial;
    int ret = 0;

    memset(build, 0, sizeof(MD_ATTRIBUTE_BUILD));

    /* If there is no backslash and no ampersand, build trivial attribute
     * without any malloc(). */
    is_trivial = TRUE;
    for(raw_off = 0; raw_off < raw_size; raw_off++) {
        if(ISANYOF3_(raw_text[raw_off], _T('\\'), _T('&'), _T('\0'))) {
            is_trivial = FALSE;
            break;
        }
    }

    if(is_trivial) {
        build->text = (CHAR*) (raw_size ? raw_text : NULL);
        build->substr_types = build->trivial_types;
        build->substr_offsets = build->trivial_offsets;
        build->substr_count = 1;
        build->substr_alloc = 0;
        build->trivial_types[0] = MD_TEXT_NORMAL;
        build->trivial_offsets[0] = 0;
        build->trivial_offsets[1] = raw_size;
        off = raw_size;
    } else {
        build->text = (CHAR*) malloc(raw_size * sizeof(CHAR));
        if(build->text == NULL) {
            MD_LOG("malloc() failed.");
            goto abort;
        }

        raw_off = 0;
        off = 0;

        while(raw_off < raw_size) {
            if(raw_text[raw_off] == _T('\0')) {
                MD_CHECK(md_build_attr_append_substr(ctx, build, MD_TEXT_NULLCHAR, off));
                memcpy(build->text + off, raw_text + raw_off, 1);
                off++;
                raw_off++;
                continue;
            }

            if(raw_text[raw_off] == _T('&')) {
                OFF ent_end;

                if(md_is_entity_str(ctx, raw_text, raw_off, raw_size, &ent_end)) {
                    MD_CHECK(md_build_attr_append_substr(ctx, build, MD_TEXT_ENTITY, off));
                    memcpy(build->text + off, raw_text + raw_off, ent_end - raw_off);
                    off += ent_end - raw_off;
                    raw_off = ent_end;
                    continue;
                }
            }

            if(build->substr_count == 0  ||  build->substr_types[build->substr_count-1] != MD_TEXT_NORMAL)
                MD_CHECK(md_build_attr_append_substr(ctx, build, MD_TEXT_NORMAL, off));

            if(!(flags & MD_BUILD_ATTR_NO_ESCAPES)  &&
               raw_text[raw_off] == _T('\\')  &&  raw_off+1 < raw_size  &&
               (ISPUNCT_(raw_text[raw_off+1]) || ISNEWLINE_(raw_text[raw_off+1])))
                raw_off++;

            build->text[off++] = raw_text[raw_off++];
        }
        build->substr_offsets[build->substr_count] = off;
    }

    attr->text = build->text;
    attr->size = off;
    attr->substr_offsets = build->substr_offsets;
    attr->substr_types = build->substr_types;
    return 0;

abort:
    md_free_attribute(ctx, build);
    return -1;
}


/*********************************************
 ***  Dictionary of Reference Definitions  ***
 *********************************************/

#define MD_FNV1A_BASE       2166136261
#define MD_FNV1A_PRIME      16777619

static inline unsigned
md_fnv1a(unsigned base, const void* data, size_t n)
{
    const unsigned char* buf = (const unsigned char*) data;
    unsigned hash = base;
    size_t i;

    for(i = 0; i < n; i++) {
        hash ^= buf[i];
        hash *= MD_FNV1A_PRIME;
    }

    return hash;
}


struct MD_REF_DEF_tag {
    CHAR* label;
    CHAR* title;
    unsigned hash;
    SZ label_size                   : 24;
    unsigned label_needs_free       :  1;
    unsigned title_needs_free       :  1;
    SZ title_size;
    OFF dest_beg;
    OFF dest_end;
};

/* Label equivalence is quite complicated with regards to whitespace and case
 * folding. This complicates computing a hash of it as well as direct comparison
 * of two labels. */

static unsigned
md_link_label_hash(const CHAR* label, SZ size)
{
    unsigned hash = MD_FNV1A_BASE;
    OFF off;
    int codepoint;
    int is_whitespace = FALSE;

    off = md_skip_unicode_whitespace(label, 0, size);
    while(off < size) {
        SZ char_size;

        codepoint = md_decode_unicode(label, off, size, &char_size);
        is_whitespace = ISUNICODEWHITESPACE_(codepoint) || ISNEWLINE_(label[off]);

        if(is_whitespace) {
            codepoint = ' ';
            hash = md_fnv1a(hash, &codepoint, 1 * sizeof(int));

            off = md_skip_unicode_whitespace(label, off, size);
        } else {
            MD_UNICODE_FOLD_INFO fold_info;

            md_get_unicode_fold_info(codepoint, &fold_info);
            hash = md_fnv1a(hash, fold_info.codepoints, fold_info.n_codepoints * sizeof(int));

            off += char_size;
        }
    }

    if(!is_whitespace) {
        codepoint = ' ';
        hash = md_fnv1a(hash, &codepoint, 1 * sizeof(int));
    }

    return hash;
}

static int
md_link_label_cmp(const CHAR* a_label, SZ a_size, const CHAR* b_label, SZ b_size)
{
    OFF a_off;
    OFF b_off;

    /* The slow path, with Unicode case folding and Unicode whitespace collapsing. */
    a_off = md_skip_unicode_whitespace(a_label, 0, a_size);
    b_off = md_skip_unicode_whitespace(b_label, 0, b_size);
    while(a_off < a_size  ||  b_off < b_size) {
        int a_codepoint, b_codepoint;
        SZ a_char_size, b_char_size;
        int a_is_whitespace, b_is_whitespace;

        if(a_off < a_size) {
            a_codepoint = md_decode_unicode(a_label, a_off, a_size, &a_char_size);
            a_is_whitespace = ISUNICODEWHITESPACE_(a_codepoint) || ISNEWLINE_(a_label[a_off]);
        } else {
            /* Treat end of label as a whitespace. */
            a_codepoint = -1;
            a_is_whitespace = TRUE;
        }

        if(b_off < b_size) {
            b_codepoint = md_decode_unicode(b_label, b_off, b_size, &b_char_size);
            b_is_whitespace = ISUNICODEWHITESPACE_(b_codepoint) || ISNEWLINE_(b_label[b_off]);
        } else {
            /* Treat end of label as a whitespace. */
            b_codepoint = -1;
            b_is_whitespace = TRUE;
        }

        if(a_is_whitespace || b_is_whitespace) {
            if(!a_is_whitespace || !b_is_whitespace)
                return (a_is_whitespace ? -1 : +1);

            a_off = md_skip_unicode_whitespace(a_label, a_off, a_size);
            b_off = md_skip_unicode_whitespace(b_label, b_off, b_size);
        } else {
            MD_UNICODE_FOLD_INFO a_fold_info, b_fold_info;
            int cmp;

            md_get_unicode_fold_info(a_codepoint, &a_fold_info);
            md_get_unicode_fold_info(b_codepoint, &b_fold_info);

            if(a_fold_info.n_codepoints != b_fold_info.n_codepoints)
                return (a_fold_info.n_codepoints - b_fold_info.n_codepoints);
            cmp = memcmp(a_fold_info.codepoints, b_fold_info.codepoints, a_fold_info.n_codepoints * sizeof(int));
            if(cmp != 0)
                return cmp;

            a_off += a_char_size;
            b_off += b_char_size;
        }
    }

    return 0;
}

typedef struct MD_REF_DEF_LIST_tag MD_REF_DEF_LIST;
struct MD_REF_DEF_LIST_tag {
    int n_ref_defs;
    int alloc_ref_defs;
    MD_REF_DEF* ref_defs[];  /* Valid items always  point into ctx->ref_defs[] */
};

static int
md_ref_def_cmp(const void* a, const void* b)
{
    const MD_REF_DEF* a_ref = *(const MD_REF_DEF**)a;
    const MD_REF_DEF* b_ref = *(const MD_REF_DEF**)b;

    if(a_ref->hash < b_ref->hash)
        return -1;
    else if(a_ref->hash > b_ref->hash)
        return +1;
    else
        return md_link_label_cmp(a_ref->label, a_ref->label_size, b_ref->label, b_ref->label_size);
}

static int
md_ref_def_cmp_stable(const void* a, const void* b)
{
    int cmp;

    cmp = md_ref_def_cmp(a, b);

    /* Ensure stability of the sorting. */
    if(cmp == 0) {
        const MD_REF_DEF* a_ref = *(const MD_REF_DEF**)a;
        const MD_REF_DEF* b_ref = *(const MD_REF_DEF**)b;

        if(a_ref < b_ref)
            cmp = -1;
        else if(a_ref > b_ref)
            cmp = +1;
        else
            cmp = 0;
    }

    return cmp;
}

static int
md_build_ref_def_hashtable(MD_CTX* ctx)
{
    int i, j;

    if(ctx->n_ref_defs == 0)
        return 0;

    ctx->ref_def_hashtable_size = (ctx->n_ref_defs * 5) / 4;
    ctx->ref_def_hashtable = malloc(ctx->ref_def_hashtable_size * sizeof(void*));
    if(ctx->ref_def_hashtable == NULL) {
        MD_LOG("malloc() failed.");
        goto abort;
    }
    memset(ctx->ref_def_hashtable, 0, ctx->ref_def_hashtable_size * sizeof(void*));

    /* Each member of ctx->ref_def_hashtable[] can be:
     *  -- NULL,
     *  -- pointer to the MD_REF_DEF in ctx->ref_defs[], or
     *  -- pointer to a MD_REF_DEF_LIST, which holds multiple pointers to
     *     such MD_REF_DEFs.
     */
    for(i = 0; i < ctx->n_ref_defs; i++) {
        MD_REF_DEF* def = &ctx->ref_defs[i];
        void* bucket;
        MD_REF_DEF_LIST* list;

        def->hash = md_link_label_hash(def->label, def->label_size);
        bucket = ctx->ref_def_hashtable[def->hash % ctx->ref_def_hashtable_size];

        if(bucket == NULL) {
            ctx->ref_def_hashtable[def->hash % ctx->ref_def_hashtable_size] = def;
            continue;
        }

        if(ctx->ref_defs <= (MD_REF_DEF*) bucket  &&  (MD_REF_DEF*) bucket < ctx->ref_defs + ctx->n_ref_defs) {
            /* The bucket already contains one ref. def. Lets see whether it
             * is the same label (ref. def. duplicate) or different one
             * (hash conflict). */
            MD_REF_DEF* old_def = (MD_REF_DEF*) bucket;

            if(md_link_label_cmp(def->label, def->label_size, old_def->label, old_def->label_size) == 0) {
                /* Ignore this ref. def. */
                continue;
            }

            /* Make the bucket capable of holding more ref. defs. */
            list = (MD_REF_DEF_LIST*) malloc(sizeof(MD_REF_DEF_LIST) + 4 * sizeof(MD_REF_DEF));
            if(list == NULL) {
                MD_LOG("malloc() failed.");
                goto abort;
            }
            list->ref_defs[0] = old_def;
            list->ref_defs[1] = def;
            list->n_ref_defs = 2;
            list->alloc_ref_defs = 4;
            ctx->ref_def_hashtable[def->hash % ctx->ref_def_hashtable_size] = list;
            continue;
        }

        /* Append the def to the bucket list. */
        list = (MD_REF_DEF_LIST*) bucket;
        if(list->n_ref_defs >= list->alloc_ref_defs) {
            MD_REF_DEF_LIST* list_tmp = (MD_REF_DEF_LIST*) realloc(list,
                        sizeof(MD_REF_DEF_LIST) + 2 * list->alloc_ref_defs * sizeof(MD_REF_DEF));
            if(list_tmp == NULL) {
                MD_LOG("realloc() failed.");
                goto abort;
            }
            list = list_tmp;
            list->alloc_ref_defs *= 2;
            ctx->ref_def_hashtable[def->hash % ctx->ref_def_hashtable_size] = list;
        }

        list->ref_defs[list->n_ref_defs] = def;
        list->n_ref_defs++;
    }

    /* Sort the complex buckets so we can use bsearch() with them. */
    for(i = 0; i < ctx->ref_def_hashtable_size; i++) {
        void* bucket = ctx->ref_def_hashtable[i];
        MD_REF_DEF_LIST* list;

        if(bucket == NULL)
            continue;
        if(ctx->ref_defs <= (MD_REF_DEF*) bucket  &&  (MD_REF_DEF*) bucket < ctx->ref_defs + ctx->n_ref_defs)
            continue;

        list = (MD_REF_DEF_LIST*) bucket;
        qsort(list->ref_defs, list->n_ref_defs, sizeof(MD_REF_DEF*), md_ref_def_cmp_stable);

        /* Disable duplicates. */
        for(j = 1; j < list->n_ref_defs; j++) {
            if(md_ref_def_cmp(&list->ref_defs[j-1], &list->ref_defs[j]) == 0)
                list->ref_defs[j] = list->ref_defs[j-1];
        }
    }

    return 0;

abort:
    return -1;
}

static void
md_free_ref_def_hashtable(MD_CTX* ctx)
{
    if(ctx->ref_def_hashtable != NULL) {
        int i;

        for(i = 0; i < ctx->ref_def_hashtable_size; i++) {
            void* bucket = ctx->ref_def_hashtable[i];
            if(bucket == NULL)
                continue;
            if(ctx->ref_defs <= (MD_REF_DEF*) bucket  &&  (MD_REF_DEF*) bucket < ctx->ref_defs + ctx->n_ref_defs)
                continue;
            free(bucket);
        }

        free(ctx->ref_def_hashtable);
    }
}

static const MD_REF_DEF*
md_lookup_ref_def(MD_CTX* ctx, const CHAR* label, SZ label_size)
{
    unsigned hash;
    void* bucket;

    if(ctx->ref_def_hashtable_size == 0)
        return NULL;

    hash = md_link_label_hash(label, label_size);
    bucket = ctx->ref_def_hashtable[hash % ctx->ref_def_hashtable_size];

    if(bucket == NULL) {
        return NULL;
    } else if(ctx->ref_defs <= (MD_REF_DEF*) bucket  &&  (MD_REF_DEF*) bucket < ctx->ref_defs + ctx->n_ref_defs) {
        const MD_REF_DEF* def = (MD_REF_DEF*) bucket;

        if(md_link_label_cmp(def->label, def->label_size, label, label_size) == 0)
            return def;
        else
            return NULL;
    } else {
        MD_REF_DEF_LIST* list = (MD_REF_DEF_LIST*) bucket;
        MD_REF_DEF key_buf;
        const MD_REF_DEF* key = &key_buf;
        const MD_REF_DEF** ret;

        key_buf.label = (CHAR*) label;
        key_buf.label_size = label_size;
        key_buf.hash = md_link_label_hash(key_buf.label, key_buf.label_size);

        ret = (const MD_REF_DEF**) bsearch(&key, list->ref_defs,
                    list->n_ref_defs, sizeof(MD_REF_DEF*), md_ref_def_cmp);
        if(ret != NULL)
            return *ret;
        else
            return NULL;
    }
}


/***************************
 ***  Recognizing Links  ***
 ***************************/

/* Note this code is partially shared between processing inlines and blocks
 * as reference definitions and links share some helper parser functions.
 */

typedef struct MD_LINK_ATTR_tag MD_LINK_ATTR;
struct MD_LINK_ATTR_tag {
    OFF dest_beg;
    OFF dest_end;

    CHAR* title;
    SZ title_size;
    int title_needs_free;
};


static int
md_is_link_label(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg,
                 OFF* p_end, int* p_beg_line_index, int* p_end_line_index,
                 OFF* p_contents_beg, OFF* p_contents_end)
{
    OFF off = beg;
    OFF contents_beg = 0;
    OFF contents_end = 0;
    int line_index = 0;
    int len = 0;

    if(CH(off) != _T('['))
        return FALSE;
    off++;

    while(1) {
        OFF line_end = lines[line_index].end;

        while(off < line_end) {
            if(CH(off) == _T('\\')  &&  off+1 < ctx->size  &&  (ISPUNCT(off+1) || ISNEWLINE(off+1))) {
                if(contents_end == 0) {
                    contents_beg = off;
                    *p_beg_line_index = line_index;
                }
                contents_end = off + 2;
                off += 2;
            } else if(CH(off) == _T('[')) {
                return FALSE;
            } else if(CH(off) == _T(']')) {
                if(contents_beg < contents_end) {
                    /* Success. */
                    *p_contents_beg = contents_beg;
                    *p_contents_end = contents_end;
                    *p_end = off+1;
                    *p_end_line_index = line_index;
                    return TRUE;
                } else {
                    /* Link label must have some non-whitespace contents. */
                    return FALSE;
                }
            } else {
                int codepoint;
                SZ char_size;

                codepoint = md_decode_unicode(ctx->text, off, ctx->size, &char_size);
                if(!ISUNICODEWHITESPACE_(codepoint)) {
                    if(contents_end == 0) {
                        contents_beg = off;
                        *p_beg_line_index = line_index;
                    }
                    contents_end = off + char_size;
                }

                off += char_size;
            }

            len++;
            if(len > 999)
                return FALSE;
        }

        line_index++;
        len++;
        if(line_index < n_lines)
            off = lines[line_index].beg;
        else
            break;
    }

    return FALSE;
}

static int
md_is_link_destination_A(MD_CTX* ctx, OFF beg, OFF max_end, OFF* p_end,
                         OFF* p_contents_beg, OFF* p_contents_end)
{
    OFF off = beg;

    if(off >= max_end  ||  CH(off) != _T('<'))
        return FALSE;
    off++;

    while(off < max_end) {
        if(CH(off) == _T('\\')  &&  off+1 < max_end  &&  ISPUNCT(off+1)) {
            off += 2;
            continue;
        }

        if(ISWHITESPACE(off)  ||  CH(off) == _T('<'))
            return FALSE;

        if(CH(off) == _T('>')) {
            /* Success. */
            *p_contents_beg = beg+1;
            *p_contents_end = off;
            *p_end = off+1;
            return TRUE;
        }

        off++;
    }

    return FALSE;
}

static int
md_is_link_destination_B(MD_CTX* ctx, OFF beg, OFF max_end, OFF* p_end,
                         OFF* p_contents_beg, OFF* p_contents_end)
{
    OFF off = beg;
    int parenthesis_level = 0;

    while(off < max_end) {
        if(CH(off) == _T('\\')  &&  off+1 < max_end  &&  ISPUNCT(off+1)) {
            off += 2;
            continue;
        }

        if(ISWHITESPACE(off) || ISCNTRL(off))
            break;

        /* Link destination may include balanced pairs of unescaped '(' ')'.
         * Note we limit the maximal nesting level by 32 to protect us from
         * https://github.com/jgm/cmark/issues/214 */
        if(CH(off) == _T('(')) {
            parenthesis_level++;
            if(parenthesis_level > 32)
                return FALSE;
        } else if(CH(off) == _T(')')) {
            if(parenthesis_level == 0)
                break;
            parenthesis_level--;
        }

        off++;
    }

    if(parenthesis_level != 0  ||  off == beg)
        return FALSE;

    /* Success. */
    *p_contents_beg = beg;
    *p_contents_end = off;
    *p_end = off;
    return TRUE;
}

static int
md_is_link_title(MD_CTX* ctx, const MD_LINE* lines, int n_lines, OFF beg,
                 OFF* p_end, int* p_beg_line_index, int* p_end_line_index,
                 OFF* p_contents_beg, OFF* p_contents_end)
{
    OFF off = beg;
    CHAR closer_char;
    int line_index = 0;

    /* Optional white space with up to one line break. */
    while(off < lines[line_index].end  &&  ISWHITESPACE(off))
        off++;
    if(off >= lines[line_index].end) {
        line_index++;
        if(line_index >= n_lines)
            return FALSE;
        off = lines[line_index].beg;
    }

    *p_beg_line_index = line_index;

    /* First char determines how to detect end of it. */
    switch(CH(off)) {
        case _T('"'):   closer_char = _T('"'); break;
        case _T('\''):  closer_char = _T('\''); break;
        case _T('('):   closer_char = _T(')'); break;
        default:        return FALSE;
    }
    off++;

    *p_contents_beg = off;

    while(line_index < n_lines) {
        OFF line_end = lines[line_index].end;

        while(off < line_end) {
            if(CH(off) == _T('\\')  &&  off+1 < ctx->size  &&  (ISPUNCT(off+1) || ISNEWLINE(off+1))) {
                off++;
            } else if(CH(off) == closer_char) {
                /* Success. */
                *p_contents_end = off;
                *p_end = off+1;
                *p_end_line_index = line_index;
                return TRUE;
            }

            off++;
        }

        line_index++;
    }

    return FALSE;
}

/* Returns 0 if it is not a reference definition.
 *
 * Returns N > 0 if it is a reference definition. N then corresponds to the
 * number of lines forming it). In this case the definition is stored for
 * resolving any links referring to it.
 *
 * Returns -1 in case of an error (out of memory).
 */
static int
md_is_link_reference_definition_helper(
            MD_CTX* ctx, const MD_LINE* lines, int n_lines,
            int (*is_link_dest_fn)(MD_CTX*, OFF, OFF, OFF*, OFF*, OFF*))
{
    OFF label_contents_beg;
    OFF label_contents_end;
    int label_contents_line_index = -1;
    int label_is_multiline;
    CHAR* label;
    SZ label_size;
    int label_needs_free = FALSE;
    OFF dest_contents_beg;
    OFF dest_contents_end;
    OFF title_contents_beg;
    OFF title_contents_end;
    int title_contents_line_index;
    int title_is_multiline;
    OFF off;
    int line_index = 0;
    int tmp_line_index;
    MD_REF_DEF* def;
    int ret;

    /* Link label. */
    if(!md_is_link_label(ctx, lines, n_lines, lines[0].beg,
                &off, &label_contents_line_index, &line_index,
                &label_contents_beg, &label_contents_end))
        return FALSE;
    label_is_multiline = (label_contents_line_index != line_index);

    /* Colon. */
    if(off >= lines[line_index].end  ||  CH(off) != _T(':'))
        return FALSE;
    off++;

    /* Optional white space with up to one line break. */
    while(off < lines[line_index].end  &&  ISWHITESPACE(off))
        off++;
    if(off >= lines[line_index].end) {
        line_index++;
        if(line_index >= n_lines)
            return FALSE;
        off = lines[line_index].beg;
    }

    /* Link destination. */
    if(!is_link_dest_fn(ctx, off, lines[line_index].end,
                        &off, &dest_contents_beg, &dest_contents_end))
        return FALSE;

    /* (Optional) title. Note we interpret it as an title only if nothing
     * more follows on its last line. */
    if(md_is_link_title(ctx, lines + line_index, n_lines - line_index, off,
                &off, &title_contents_line_index, &tmp_line_index,
                &title_contents_beg, &title_contents_end)
        &&  off >= lines[line_index + tmp_line_index].end)
    {
        title_is_multiline = (tmp_line_index != title_contents_line_index);
        title_contents_line_index += line_index;
        line_index += tmp_line_index;
    } else {
        /* Not a title. */
        title_is_multiline = FALSE;
        title_contents_beg = off;
        title_contents_end = off;
        title_contents_line_index = 0;
    }

    /* Nothing more can follow on the last line. */
    if(off < lines[line_index].end)
        return FALSE;

    /* Construct label. */
    if(!label_is_multiline) {
        label = (CHAR*) STR(label_contents_beg);
        label_size = label_contents_end - label_contents_beg;
        label_needs_free = FALSE;
    } else {
        MD_CHECK(md_merge_lines_alloc(ctx, label_contents_beg, label_contents_end,
                    lines + label_contents_line_index, n_lines - label_contents_line_index,
                    _T(' '), &label, &label_size));
        label_needs_free = TRUE;
    }

    /* Store the reference definition. */
    if(ctx->n_ref_defs >= ctx->alloc_ref_defs) {
        MD_REF_DEF* new_defs;

        ctx->alloc_ref_defs = (ctx->alloc_ref_defs > 0 ? ctx->alloc_ref_defs * 2 : 16);
        new_defs = (MD_REF_DEF*) realloc(ctx->ref_defs, ctx->alloc_ref_defs * sizeof(MD_REF_DEF));
        if(new_defs == NULL) {
            MD_LOG("realloc() failed.");
            ret = -1;
            goto abort;
        }

        ctx->ref_defs = new_defs;
    }

    def = &ctx->ref_defs[ctx->n_ref_defs];
    memset(def, 0, sizeof(MD_REF_DEF));

    def->label = label;
    def->label_size = label_size;
    def->label_needs_free = label_needs_free;

    def->dest_beg = dest_contents_beg;
    def->dest_end = dest_contents_end;

    if(title_contents_beg >= title_contents_end) {
        def->title = NULL;
        def->title_size = 0;
    } else if(!title_is_multiline) {
        def->title = (CHAR*) STR(title_contents_beg);
        def->title_size = title_contents_end - title_contents_beg;
    } else {
        MD_CHECK(md_merge_lines_alloc(ctx, title_contents_beg, title_contents_end,
                    lines + title_contents_line_index, n_lines - title_contents_line_index,
                    _T('\n'), &def->title, &def->title_size));
        def->title_needs_free = TRUE;
    }

    /* Success. */
    ctx->n_ref_defs++;
    return line_index + 1;

abort:
    /* Failure. */
    if(label_needs_free)
        free(label);
    return -1;
}

static inline int
md_is_link_reference_definition(MD_CTX* ctx, const MD_LINE* lines, int n_lines)
{
    int ret;
    ret = md_is_link_reference_definition_helper(ctx, lines, n_lines, md_is_link_destination_A);
    if(ret == 0)
        ret = md_is_link_reference_definition_helper(ctx, lines, n_lines, md_is_link_destination_B);
    return ret;
}

static int
md_is_link_reference(MD_CTX* ctx, const MD_LINE* lines, int n_lines,
                     OFF beg, OFF end, MD_LINK_ATTR* attr)
{
    const MD_REF_DEF* def;
    const MD_LINE* beg_line;
    const MD_LINE* end_line;
    CHAR* label;
    SZ label_size;
    int ret;

    MD_ASSERT(CH(beg) == _T('[') || CH(beg) == _T('!'));
    MD_ASSERT(CH(end-1) == _T(']'));

    beg += (CH(beg) == _T('!') ? 2 : 1);
    end--;

    /* Find lines corresponding to the beg and end positions. */
    MD_ASSERT(lines[0].beg <= beg);
    beg_line = lines;
    while(beg >= beg_line->end)
        beg_line++;

    MD_ASSERT(end <= lines[n_lines-1].end);
    end_line = beg_line;
    while(end >= end_line->end)
        end_line++;

    if(beg_line != end_line) {
        MD_CHECK(md_merge_lines_alloc(ctx, beg, end, beg_line,
                 n_lines - (beg_line - lines), _T(' '), &label, &label_size));
    } else {
        label = (CHAR*) STR(beg);
        label_size = end - beg;
    }

    def = md_lookup_ref_def(ctx, label, label_size);
    if(def != NULL) {
        attr->dest_beg = def->dest_beg;
        attr->dest_end = def->dest_end;
        attr->title = def->title;
        attr->title_size = def->title_size;
        attr->title_needs_free = FALSE;
    }

    if(beg_line != end_line)
        free(label);

    ret = (def != NULL);

abort:
    return ret;
}

static int
md_is_inline_link_spec_helper(MD_CTX* ctx, const MD_LINE* lines, int n_lines,
                              OFF beg, OFF* p_end, MD_LINK_ATTR* attr,
                              int (*is_link_dest_fn)(MD_CTX*, OFF, OFF, OFF*, OFF*, OFF*))
{
    int line_index = 0;
    int tmp_line_index;
    OFF title_contents_beg;
    OFF title_contents_end;
    int title_contents_line_index;
    int title_is_multiline;
    OFF off = beg;
    int ret = FALSE;

    while(off >= lines[line_index].end)
        line_index++;

    MD_ASSERT(CH(off) == _T('('));
    off++;

    /* Optional white space with up to one line break. */
    while(off < lines[line_index].end  &&  ISWHITESPACE(off))
        off++;
    if(off >= lines[line_index].end  &&  ISNEWLINE(off)) {
        line_index++;
        if(line_index >= n_lines)
            return FALSE;
        off = lines[line_index].beg;
    }

    /* Link destination may be omitted, but only when not also having a title. */
    if(off < ctx->size  &&  CH(off) == _T(')')) {
        attr->dest_beg = off;
        attr->dest_end = off;
        attr->title = NULL;
        attr->title_size = 0;
        attr->title_needs_free = FALSE;
        off++;
        *p_end = off;
        return TRUE;
    }

    /* Link destination. */
    if(!is_link_dest_fn(ctx, off, lines[line_index].end,
                        &off, &attr->dest_beg, &attr->dest_end))
        return FALSE;

    /* (Optional) title. */
    if(md_is_link_title(ctx, lines + line_index, n_lines - line_index, off,
                &off, &title_contents_line_index, &tmp_line_index,
                &title_contents_beg, &title_contents_end))
    {
        title_is_multiline = (tmp_line_index != title_contents_line_index);
        title_contents_line_index += line_index;
        line_index += tmp_line_index;
    } else {
        /* Not a title. */
        title_is_multiline = FALSE;
        title_contents_beg = off;
        title_contents_end = off;
        title_contents_line_index = 0;
    }

    /* Optional whitespace followed with final ')'. */
    while(off < lines[line_index].end  &&  ISWHITESPACE(off))
        off++;
    if(off >= lines[line_index].end  &&  ISNEWLINE(off)) {
        line_index++;
        if(line_index >= n_lines)
            return FALSE;
        off = lines[line_index].beg;
    }
    if(CH(off) != _T(')'))
        goto abort;
    off++;

    if(title_contents_beg >= title_contents_end) {
        attr->title = NULL;
        attr->title_size = 0;
        attr->title_needs_free = FALSE;
    } else if(!title_is_multiline) {
        attr->title = (CHAR*) STR(title_contents_beg);
        attr->title_size = title_contents_end - title_contents_beg;
        attr->title_needs_free = FALSE;
    } else {
        MD_CHECK(md_merge_lines_alloc(ctx, title_contents_beg, title_contents_end,
                    lines + title_contents_line_index, n_lines - title_contents_line_index,
                    _T('\n'), &attr->title, &attr->title_size));
        attr->title_needs_free = TRUE;
    }

    *p_end = off;
    ret = TRUE;

abort:
    return ret;
}

static inline int
md_is_inline_link_spec(MD_CTX* ctx, const MD_LINE* lines, int n_lines,
                       OFF beg, OFF* p_end, MD_LINK_ATTR* attr)
{
    return md_is_inline_link_spec_helper(ctx, lines, n_lines, beg, p_end, attr, md_is_link_destination_A) ||
           md_is_inline_link_spec_helper(ctx, lines, n_lines, beg, p_end, attr, md_is_link_destination_B);
}

static void
md_free_ref_defs(MD_CTX* ctx)
{
    int i;

    for(i = 0; i < ctx->n_ref_defs; i++) {
        MD_REF_DEF* def = &ctx->ref_defs[i];

        if(def->label_needs_free)
            free(def->label);
        if(def->title_needs_free)
            free(def->title);
    }

    free(ctx->ref_defs);
}


/******************************************
 ***  Processing Inlines (a.k.a Spans)  ***
 ******************************************/

/* We process inlines in few phases:
 *
 * (1) We go through the block text and collect all significant characters
 *     which may start/end a span or some other significant position into
 *     ctx->marks[]. Core of this is what md_collect_marks() does.
 *
 *     We also do some very brief preliminary context-less analysis, whether
 *     it might be opener or closer (e.g. of an emphasis span).
 *
 *     This speeds the other steps as we do not need to re-iterate over all
 *     characters anymore.
 *
 * (2) We analyze each potential mark types, in order by their precedence.
 *
 *     In each md_analyze_XXX() function, we re-iterate list of the marks,
 *     skipping already resolved regions (in preceding precedences) and try to
 *     resolve them.
 *
 * (2.1) For trivial marks, which are single (e.g. HTML entity), we just mark
 *       them as resolved.
 *
 * (2.2) For range-type marks, we analyze whether the mark could be closer
 *       and, if yes, whether there is some preceding opener it could satisfy.
 *
 *       If not we check whether it could be really an opener and if yes, we
 *       remember it so subsequent closers may resolve it.
 *
 * (3) Finally, when all marks were analyzed, we render the block contents
 *     by calling MD_RENDERER::text() callback, interrupting by ::enter_span()
 *     or ::close_span() whenever we reach a resolved mark.
 */


/* The mark structure.
 *
 * '\\': Maybe escape sequence.
 * '\0': NULL char.
 *  '*': Maybe (strong) emphasis start/end.
 *  '_': Maybe (strong) emphasis start/end.
 *  '~': Maybe strikethrough start/end (needs MD_FLAG_STRIKETHROUGH).
 *  '`': Maybe code span start/end.
 *  '&': Maybe start of entity.
 *  ';': Maybe end of entity.
 *  '<': Maybe start of raw HTML or autolink.
 *  '>': Maybe end of raw HTML or autolink.
 *  '[': Maybe start of link label or link text.
 *  '!': Equivalent of '[' for image.
 *  ']': Maybe end of link label or link text.
 *  '@': Maybe permissive e-mail auto-link (needs MD_FLAG_PERMISSIVEEMAILAUTOLINKS).
 *  ':': Maybe permissive URL auto-link (needs MD_FLAG_PERMISSIVEURLAUTOLINKS).
 *  '.': Maybe permissive WWW auto-link (needs MD_FLAG_PERMISSIVEWWWAUTOLINKS).
 *  'D': Dummy mark, it reserves a space for splitting a previous mark
 *       (e.g. emphasis) or to make more space for storing some special data
 *       related to the preceding mark (e.g. link).
 *
 * Note that not all instances of these chars in the text imply creation of the
 * structure. Only those which have (or may have, after we see more context)
 * the special meaning.
 *
 * (Keep this struct as small as possible to fit as much of them into CPU
 * cache line.)
 */
struct MD_MARK_tag {
    OFF beg;
    OFF end;

    /* For unresolved openers, 'prev' and 'next' form the chain of open openers
     * of given type 'ch'.
     *
     * During resolving, we disconnect from the chain and point to the
     * corresponding counterpart so opener points to its closer and vice versa.
     */
    int prev;
    int next;
    CHAR ch;
    unsigned char flags;
};

/* Mark flags (these apply to ALL mark types). */
#define MD_MARK_POTENTIAL_OPENER            0x01  /* Maybe opener. */
#define MD_MARK_POTENTIAL_CLOSER            0x02  /* Maybe closer. */
#define MD_MARK_OPENER                      0x04  /* Definitely opener. */
#define MD_MARK_CLOSER                      0x08  /* Definitely closer. */
#define MD_MARK_RESOLVED                    0x10  /* Resolved in any definite way. */

/* Mark flags specific for various mark types (so they can share bits). */
#define MD_MARK_EMPH_INTRAWORD              0x20  /* Helper for the "rule of 3". */
#define MD_MARK_EMPH_MODULO3_0              0x40
#define MD_MARK_EMPH_MODULO3_1              0x80
#define MD_MARK_EMPH_MODULO3_2              (0x40 | 0x80)
#define MD_MARK_EMPH_MODULO3_MASK           (0x40 | 0x80)
#define MD_MARK_AUTOLINK                    0x20  /* Distinguisher for '<', '>'. */
#define MD_MARK_VALIDPERMISSIVEAUTOLINK     0x20  /* For permissive autolinks. */


static MD_MARK*
md_push_mark(MD_CTX* ctx)
{
    if(ctx->n_marks >= ctx->alloc_marks) {
        MD_MARK* new_marks;

        ctx->alloc_marks = (ctx->alloc_marks > 0 ? ctx->alloc_marks * 2 : 64);
        new_marks = realloc(ctx->marks, ctx->alloc_marks * sizeof(MD_MARK));
        if(new_marks == NULL) {
            MD_LOG("realloc() failed.");
            return NULL;
        }

        ctx->marks = new_marks;
    }

    return &ctx->marks[ctx->n_marks++];
}

#define PUSH_MARK_()                                                    \
        do {                                                            \
            mark = md_push_mark(ctx);                                   \
            if(mark == NULL) {                                          \
                ret = -1;                                               \
                goto abort;                                             \
            }                                                           \
        } while(0)

#define PUSH_MARK(ch_, beg_, end_, flags_)                              \
        do {                                                            \
            PUSH_MARK_();                                               \
            mark->beg = (beg_);                                         \
            mark->end = (end_);                                         \
            mark->prev = -1;                                            \
            mark->next = -1;                                            \
            mark->ch = (char)(ch_);                                     \
            mark->flags = (flags_);                                     \
        } while(0)


static void
md_mark_chain_append(MD_CTX* ctx, MD_MARKCHAIN* chain, int mark_index)
{
    if(chain->tail >= 0)
        ctx->marks[chain->tail].next = mark_index;
    else
        chain->head = mark_index;

    ctx->marks[mark_index].prev = chain->tail;
    chain->tail = mark_index;
}

/* Sometimes, we need to store a pointer into the mark. It is quite rare
 * so we do not bother to make MD_MARK use union, and it can only happen
 * for dummy marks. */
static inline void
md_mark_store_ptr(MD_CTX* ctx, int mark_index, void* ptr)
{
    MD_MARK* mark = &ctx->marks[mark_index];
    MD_ASSERT(mark->ch == 'D');

    /* Check only members beg and end are misused for this. */
    MD_ASSERT(sizeof(void*) <= 2 * sizeof(OFF));
    memcpy(mark, &ptr, sizeof(void*));
}

static inline void*
md_mark_get_ptr(MD_CTX* ctx, int mark_index)
{
    void* ptr;
    MD_MARK* mark = &ctx->marks[mark_index];
    MD_ASSERT(mark->ch == 'D');
    memcpy(&ptr, mark, sizeof(void*));
    return ptr;
}

static void
md_resolve_range(MD_CTX* ctx, MD_MARKCHAIN* chain, int opener_index, int closer_index)
{
    MD_MARK* opener = &ctx->marks[opener_index];
    MD_MARK* closer = &ctx->marks[closer_index];

    /* Remove opener from the list of openers. */
    if(chain != NULL) {
        if(opener->prev >= 0)
            ctx->marks[opener->prev].next = opener->next;
        else
            chain->head = opener->next;

        if(opener->next >= 0)
            ctx->marks[opener->next].prev = opener->prev;
        else
            chain->tail = opener->prev;
    }

    /* Interconnect opener and closer and mark both as resolved. */
    opener->next = closer_index;
    opener->flags |= MD_MARK_OPENER | MD_MARK_RESOLVED;
    closer->prev = opener_index;
    closer->flags |= MD_MARK_CLOSER | MD_MARK_RESOLVED;
}


#define MD_ROLLBACK_ALL         0
#define MD_ROLLBACK_CROSSING    1

/* In the range ctx->marks[opener_index] ... [closer_index], undo some or all
 * resolvings accordingly to these rules:
 *
 * (1) All openers BEFORE the range corresponding to any closer inside the
 *     range are un-resolved and they are re-added to their respective chains
 *     of unresolved openers. This ensures we can reuse the opener for closers
 *     AFTER the range.
 *
 * (2) If 'how' is MD_ROLLBACK_ALL, then ALL resolved marks inside the range
 *     are discarded.
 *
 * (3) If 'how' is MD_ROLLBACK_CROSSING, only closers with openers handled
 *     in (1) are discarded. I.e. pairs of openers and closers which are both
 *     inside the range are retained as well as any unpaired marks.
 */
static void
md_rollback(MD_CTX* ctx, int opener_index, int closer_index, int how)
{
    int i;
    int mark_index;

    /* Cut all unresolved openers at the mark index. */
    for(i = OPENERS_CHAIN_FIRST; i < OPENERS_CHAIN_LAST+1; i++) {
        MD_MARKCHAIN* chain = &ctx->mark_chains[i];

        while(chain->tail >= opener_index)
            chain->tail = ctx->marks[chain->tail].prev;

        if(chain->tail >= 0)
            ctx->marks[chain->tail].next = -1;
        else
            chain->head = -1;
    }

    /* Go backwards so that un-resolved openers are re-added into their
     * respective chains, in the right order. */
    mark_index = closer_index - 1;
    while(mark_index > opener_index) {
        MD_MARK* mark = &ctx->marks[mark_index];
        int mark_flags = mark->flags;
        int discard_flag = (how == MD_ROLLBACK_ALL);

        if(mark->flags & MD_MARK_CLOSER) {
            int mark_opener_index = mark->prev;

            /* Undo opener BEFORE the range. */
            if(mark_opener_index < opener_index) {
                MD_MARK* mark_opener = &ctx->marks[mark_opener_index];
                MD_MARKCHAIN* chain;

                mark_opener->flags &= ~(MD_MARK_OPENER | MD_MARK_CLOSER | MD_MARK_RESOLVED);

                switch(mark_opener->ch) {
                    case '*':   chain = &ASTERISK_OPENERS; break;
                    case '_':   chain = &UNDERSCORE_OPENERS; break;
                    case '`':   chain = &BACKTICK_OPENERS; break;
                    case '<':   chain = &LOWERTHEN_OPENERS; break;
                    case '~':   chain = &TILDE_OPENERS; break;
                    default:    MD_UNREACHABLE(); break;
                }
                md_mark_chain_append(ctx, chain, mark_opener_index);

                discard_flag = 1;
            }
        }

        /* And reset our flags. */
        if(discard_flag)
            mark->flags &= ~(MD_MARK_OPENER | MD_MARK_CLOSER | MD_MARK_RESOLVED);

        /* Jump as far as we can over unresolved or non-interesting marks. */
        switch(how) {
            case MD_ROLLBACK_CROSSING:
                if((mark_flags & MD_MARK_CLOSER)  &&  mark->prev > opener_index) {
                    /* If we are closer with opener INSIDE the range, there may
                     * not be any other crosser inside the subrange. */
                    mark_index = mark->prev;
                    break;
                }
                /* Pass through. */
            default:
                mark_index--;
                break;
        }
    }
}

static void
md_build_mark_char_map(MD_CTX* ctx)
{
    memset(ctx->mark_char_map, 0, sizeof(ctx->mark_char_map));

    ctx->mark_char_map['\\'] = 1;
    ctx->mark_char_map['*'] = 1;
    ctx->mark_char_map['_'] = 1;
    ctx->mark_char_map['`'] = 1;
    ctx->mark_char_map['&'] = 1;
    ctx->mark_char_map[';'] = 1;
    ctx->mark_char_map['<'] = 1;
    ctx->mark_char_map['>'] = 1;
    ctx->mark_char_map['['] = 1;
    ctx->mark_char_map['!'] = 1;
    ctx->mark_char_map[']'] = 1;
    ctx->mark_char_map['\0'] = 1;

    if(ctx->parser.flags & MD_FLAG_STRIKETHROUGH)
        ctx->mark_char_map['~'] = 1;

    if(ctx->parser.flags & MD_FLAG_PERMISSIVEEMAILAUTOLINKS)
        ctx->mark_char_map['@'] = 1;

    if(ctx->parser.flags & MD_FLAG_PERMISSIVEURLAUTOLINKS)
        ctx->mark_char_map[':'] = 1;

    if(ctx->parser.flags & MD_FLAG_PERMISSIVEWWWAUTOLINKS)
        ctx->mark_char_map['.'] = 1;

    if(ctx->parser.flags & MD_FLAG_TABLES)
        ctx->mark_char_map['|'] = 1;

    if(ctx->parser.flags & MD_FLAG_COLLAPSEWHITESPACE) {
        int i;

        for(i = 0; i < sizeof(ctx->mark_char_map); i++) {
            if(ISWHITESPACE_(i))
                ctx->mark_char_map[i] = 1;
        }
    }
}

static int
md_collect_marks(MD_CTX* ctx, const MD_LINE* lines, int n_lines, int table_mode)
{
    int i;
    int ret = 0;
    MD_MARK* mark;

    for(i = 0; i < n_lines; i++) {
        const MD_LINE* line = &lines[i];
        OFF off = line->beg;
        OFF line_end = line->end;

        while(TRUE) {
            CHAR ch;

#ifdef MD4C_USE_UTF16
    /* For UTF-16, mark_char_map[] covers only ASCII. */
    #define IS_MARK_CHAR(off)   ((CH(off) < SIZEOF_ARRAY(ctx->mark_char_map))  &&  \
                                (ctx->mark_char_map[(unsigned char) CH(off)]))
#else
    /* For 8-bit encodings, mark_char_map[] covers all 256 elements. */
    #define IS_MARK_CHAR(off)   (ctx->mark_char_map[(unsigned char) CH(off)])
#endif

            /* Optimization: Fast path (with some loop unrolling). */
            while(off + 4 < line_end  &&  !IS_MARK_CHAR(off+0)  &&  !IS_MARK_CHAR(off+1)
                                      &&  !IS_MARK_CHAR(off+2)  &&  !IS_MARK_CHAR(off+3))
                off += 4;
            while(off < line_end  &&  !IS_MARK_CHAR(off+0))
                off++;

            if(off >= line_end)
                break;

            ch = CH(off);

            /* A backslash escape.
             * It can go beyond line->end as it may involve escaped new
             * line to form a hard break. */
            if(ch == _T('\\')  &&  off+1 < ctx->size  &&  (ISPUNCT(off+1) || ISNEWLINE(off+1))) {
                /* Hard-break cannot be on the last line of the block. */
                if(!ISNEWLINE(off+1)  ||  i+1 < n_lines)
                    PUSH_MARK(ch, off, off+2, MD_MARK_RESOLVED);

                /* If '`' or '>' follows, we need both marks as the backslash
                 * may be inside a code span or an autolink where escaping is
                 * disabled. */
                if(CH(off+1) == _T('`') || CH(off+1) == _T('>'))
                    off++;
                else
                    off += 2;
                continue;
            }

            /* A potential (string) emphasis start/end. */
            if(ch == _T('*')  ||  ch == _T('_')) {
                OFF tmp = off+1;
                int left_level;     /* What precedes: 0 = whitespace; 1 = punctuation; 2 = other char. */
                int right_level;    /* What follows: 0 = whitespace; 1 = punctuation; 2 = other char. */

                while(tmp < line_end  &&  CH(tmp) == ch)
                    tmp++;

                if(off == line->beg  ||  ISUNICODEWHITESPACEBEFORE(off))
                    left_level = 0;
                else if(ISUNICODEPUNCTBEFORE(off))
                    left_level = 1;
                else
                    left_level = 2;

                if(tmp == line_end  ||  ISUNICODEWHITESPACE(tmp))
                    right_level = 0;
                else if(ISUNICODEPUNCT(tmp))
                    right_level = 1;
                else
                    right_level = 2;

                /* Intra-word underscore doesn't have special meaning. */
                if(ch == _T('_')  &&  left_level == 2  &&  right_level == 2) {
                    left_level = 0;
                    right_level = 0;
                }

                if(left_level != 0  ||  right_level != 0) {
                    unsigned flags = 0;

                    if(left_level > 0  &&  left_level >= right_level)
                        flags |= MD_MARK_POTENTIAL_CLOSER;
                    if(right_level > 0  &&  right_level >= left_level)
                        flags |= MD_MARK_POTENTIAL_OPENER;
                    if(left_level == 2  &&  right_level == 2)
                        flags |= MD_MARK_EMPH_INTRAWORD;

                    /* For "the rule of three" we need to remember the original
                     * size of the mark (modulo three), before we potentially
                     * split the mark when being later resolved partially by some
                     * shorter closer. */
                    switch((tmp - off) % 3) {
                        case 0: flags |= MD_MARK_EMPH_MODULO3_0; break;
                        case 1: flags |= MD_MARK_EMPH_MODULO3_1; break;
                        case 2: flags |= MD_MARK_EMPH_MODULO3_2; break;
                    }

                    PUSH_MARK(ch, off, tmp, flags);

                    /* During resolving, multiple asterisks may have to be
                     * split into independent span start/ends. Consider e.g.
                     * "**foo* bar*". Therefore we push also some empty dummy
                     * marks to have enough space for that. */
                    off++;
                    while(off < tmp) {
                        PUSH_MARK('D', off, off, 0);
                        off++;
                    }
                    continue;
                }

                off = tmp;
                continue;
            }

            /* A potential code span start/end. */
            if(ch == _T('`')) {
                OFF tmp = off+1;

                while(tmp < line_end  &&  CH(tmp) == _T('`'))
                    tmp++;

                /* We limit code span marks to lower then 256 backticks. This
                 * solves a pathologic case of too many openers, each of
                 * different length: Their resolving is then O(n^2). */
                if(tmp - off < 256)
                    PUSH_MARK(ch, off, tmp, MD_MARK_POTENTIAL_OPENER | MD_MARK_POTENTIAL_CLOSER);

                off = tmp;
                continue;
            }

            /* A potential entity start. */
            if(ch == _T('&')) {
                PUSH_MARK(ch, off, off+1, MD_MARK_POTENTIAL_OPENER);
                off++;
                continue;
            }

            /* A potential entity end. */
            if(ch == _T(';')) {
                /* We surely cannot be entity unless the previous mark is '&'. */
                if(ctx->n_marks > 0  &&  ctx->marks[ctx->n_marks-1].ch == _T('&'))
                    PUSH_MARK(ch, off, off+1, MD_MARK_POTENTIAL_CLOSER);

                off++;
                continue;
            }

            /* A potential autolink or raw HTML start/end. */
            if(ch == _T('<') || ch == _T('>')) {
                if(!(ctx->parser.flags & MD_FLAG_NOHTMLSPANS))
                    PUSH_MARK(ch, off, off+1, (ch == _T('<') ? MD_MARK_POTENTIAL_OPENER : MD_MARK_POTENTIAL_CLOSER));

                off++;
                continue;
            }

            /* A potential link or its part. */
            if(ch == _T('[')  ||  (ch == _T('!') && off+1 < line_end && CH(off+1) == _T('['))) {
                OFF tmp = (ch == _T('[') ? off+1 : off+2);
                PUSH_MARK(ch, off, tmp, MD_MARK_POTENTIAL_OPENER);
                off = tmp;
                /* Two dummies to make enough place for data we need if it is
                 * a link. */
                PUSH_MARK('D', off, off, 0);
                PUSH_MARK('D', off, off, 0);
                continue;
            }
            if(ch == _T(']')) {
                PUSH_MARK(ch, off, off+1, MD_MARK_POTENTIAL_CLOSER);
                off++;
                continue;
            }

            /* A potential permissive e-mail autolink. */
            if(ch == _T('@')) {
                if(line->beg + 1 <= off  &&  ISALNUM(off-1)  &&
                    off + 3 < line->end  &&  ISALNUM(off+1))
                {
                    PUSH_MARK(ch, off, off+1, MD_MARK_POTENTIAL_OPENER);
                    /* Push a dummy as a reserve for a closer. */
                    PUSH_MARK('D', off, off, 0);
                }

                off++;
                continue;
            }

            /* A potential permissive URL autolink. */
            if(ch == _T(':')) {
                static struct {
                    const CHAR* scheme;
                    SZ scheme_size;
                    const CHAR* suffix;
                    SZ suffix_size;
                } scheme_map[] = {
                    /* In the order from the most frequently used, arguably. */
                    { _T("http"), 4,    _T("//"), 2 },
                    { _T("https"), 5,   _T("//"), 2 },
                    { _T("ftp"), 3,     _T("//"), 2 }
                };
                int scheme_index;

                for(scheme_index = 0; scheme_index < SIZEOF_ARRAY(scheme_map); scheme_index++) {
                    const CHAR* scheme = scheme_map[scheme_index].scheme;
                    const SZ scheme_size = scheme_map[scheme_index].scheme_size;
                    const CHAR* suffix = scheme_map[scheme_index].suffix;
                    const SZ suffix_size = scheme_map[scheme_index].suffix_size;

                    if(line->beg + scheme_size <= off  &&  md_ascii_eq(STR(off-scheme_size), scheme, scheme_size)  &&
                        (line->beg + scheme_size == off || ISWHITESPACE(off-scheme_size-1) || ISANYOF(off-scheme_size-1, _T("*_~([")))  &&
                        off + 1 + suffix_size < line->end  &&  md_ascii_eq(STR(off+1), suffix, suffix_size))
                    {
                        PUSH_MARK(ch, off-scheme_size, off+1+suffix_size, MD_MARK_POTENTIAL_OPENER);
                        /* Push a dummy as a reserve for a closer. */
                        PUSH_MARK('D', off, off, 0);
                        off += 1 + suffix_size;
                        continue;
                    }
                }

                off++;
                continue;
            }

            /* A potential permissive WWW autolink. */
            if(ch == _T('.')) {
                if(line->beg + 3 <= off  &&  md_ascii_eq(STR(off-3), _T("www"), 3)  &&
                    (line->beg + 3 == off || ISWHITESPACE(off-4) || ISANYOF(off-4, _T("*_~([")))  &&
                    off + 1 < line_end)
                {
                    PUSH_MARK(ch, off-3, off+1, MD_MARK_POTENTIAL_OPENER);
                    /* Push a dummy as a reserve for a closer. */
                    PUSH_MARK('D', off, off, 0);
                    off++;
                    continue;
                }

                off++;
                continue;
            }

            /* A potential table cell boundary. */
            if(table_mode  &&  ch == _T('|')) {
                PUSH_MARK(ch, off, off+1, 0);
                off++;
                continue;
            }

            /* A potential strikethrough start/end. */
            if(ch == _T('~')) {
                OFF tmp = off+1;

                while(tmp < line_end  &&  CH(tmp) == _T('~'))
                    tmp++;

                PUSH_MARK(ch, off, tmp, MD_MARK_POTENTIAL_OPENER | MD_MARK_POTENTIAL_CLOSER);
                off = tmp;
            }

            /* Turn non-trivial whitespace into single space. */
            if(ISWHITESPACE_(ch)) {
                OFF tmp = off+1;

                while(tmp < line_end  &&  ISWHITESPACE(tmp))
                    tmp++;

                if(tmp - off > 1  ||  ch != _T(' '))
                    PUSH_MARK(ch, off, tmp, MD_MARK_RESOLVED);

                off = tmp;
                continue;
            }

            /* NULL character. */
            if(ch == _T('\0')) {
                PUSH_MARK(ch, off, off+1, MD_MARK_RESOLVED);
                off++;
                continue;
            }

            off++;
        }
    }

    /* Add a dummy mark at the end of the mark vector to simplify
     * process_inlines(). */
    PUSH_MARK(127, ctx->size, ctx->size, MD_MARK_RESOLVED);

abort:
    return ret;
}


/* Analyze whether the back-tick is really start/end mark of a code span.
 * If yes, reset all marks inside of it and setup flags of both marks. */
static void
md_analyze_backtick(MD_CTX* ctx, int mark_index)
{
    MD_MARK* mark = &ctx->marks[mark_index];
    int opener_index = BACKTICK_OPENERS.tail;

    /* Try to find unresolved opener of the same length. If we find it,
     * we form a code span. */
    while(opener_index >= 0) {
        MD_MARK* opener = &ctx->marks[opener_index];

        if(opener->end - opener->beg == mark->end - mark->beg) {
            /* Rollback anything found inside it.
             * (e.g. the code span contains some back-ticks or other special
             * chars we misinterpreted.) */
            md_rollback(ctx, opener_index, mark_index, MD_ROLLBACK_ALL);

            /* Resolve the span. */
            md_resolve_range(ctx, &BACKTICK_OPENERS, opener_index, mark_index);

            /* Append any space or new line inside the span into the mark
             * itself to swallow it. */
            while(CH(opener->end) == _T(' ')  ||  ISNEWLINE(opener->end))
                opener->end++;
            if(mark->beg > opener->end) {
                while(CH(mark->beg-1) == _T(' ')  ||  ISNEWLINE(mark->beg-1))
                    mark->beg--;
            }

            /* Done. */
            return;
        }

        opener_index = ctx->marks[opener_index].prev;
    }

    /* We didn't find any matching opener, so we ourselves may be the opener
     * of some upcoming closer. We also have to handle specially if there is
     * a backslash mark before it as that can cancel the first backtick. */
    if(mark_index > 0  &&  (mark-1)->beg == mark->beg - 1  &&  (mark-1)->ch == '\\') {
        if(mark->end - mark->beg == 1) {
            /* Single escaped backtick. */
            return;
        }

        /* Remove the escaped backtick from the opener. */
        mark->beg++;
    }

    if(mark->flags & MD_MARK_POTENTIAL_OPENER)
        md_mark_chain_append(ctx, &BACKTICK_OPENERS, mark_index);
}

static int
md_is_autolink_uri(MD_CTX* ctx, OFF beg, OFF end)
{
    OFF off = beg;

    /* Check for scheme. */
    if(off >= end  ||  !ISASCII(off))
        return FALSE;
    off++;
    while(1) {
        if(off >= end)
            return FALSE;
        if(off - beg > 32)
            return FALSE;
        if(CH(off) == _T(':')  &&  off - beg >= 2)
            break;
        if(!ISALNUM(off) && CH(off) != _T('+') && CH(off) != _T('-') && CH(off) != _T('.'))
            return FALSE;
        off++;
    }

    /* Check the path after the scheme. */
    while(off < end) {
        if(ISWHITESPACE(off) || ISCNTRL(off) || CH(off) == _T('<') || CH(off) == _T('>'))
            return FALSE;
        off++;
    }

    return TRUE;
}

static int
md_is_autolink_email(MD_CTX* ctx, OFF beg, OFF end)
{
    OFF off = beg;
    int label_len;

    /* The code should correspond to this regexp:
            /^[a-zA-Z0-9.!#$%&'*+\/=?^_`{|}~-]+
            @[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?
            (?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/
     */

    /* Username (before '@'). */
    while(off < end  &&  (ISALNUM(off) || ISANYOF(off, _T(".!#$%&'*+/=?^_`{|}~-"))))
        off++;
    if(off <= beg)
        return FALSE;

    /* '@' */
    if(off >= end  ||  CH(off) != _T('@'))
        return FALSE;
    off++;

    /* Labels delimited with '.'; each label is sequence of 1 - 62 alnum
     * characters or '-', but '-' is not allowed as first or last char. */
    label_len = 0;
    while(off < end) {
        if(ISALNUM(off))
            label_len++;
        else if(CH(off) == _T('-')  &&  label_len > 0)
            label_len++;
        else if(CH(off) == _T('.')  &&  label_len > 0  &&  CH(off-1) != _T('-'))
            label_len = 0;
        else
            return FALSE;

        if(label_len > 63)
            return FALSE;

        off++;
    }

    if(label_len <= 0  ||  CH(off-1) == _T('-'))
        return FALSE;

    return TRUE;
}

static int
md_is_autolink(MD_CTX* ctx, OFF beg, OFF end, int* p_missing_mailto)
{
    MD_ASSERT(CH(beg) == _T('<'));
    MD_ASSERT(CH(end-1) == _T('>'));

    beg++;
    end--;

    if(md_is_autolink_uri(ctx, beg, end))
        return TRUE;

    if(md_is_autolink_email(ctx, beg, end)) {
        *p_missing_mailto = 1;
        return TRUE;
    }

    return FALSE;
}

static void
md_analyze_lt_gt(MD_CTX* ctx, int mark_index, const MD_LINE* lines, int n_lines)
{
    MD_MARK* mark = &ctx->marks[mark_index];
    int opener_index;

    /* If it is an opener ('<'), remember it. */
    if(mark->flags & MD_MARK_POTENTIAL_OPENER) {
        md_mark_chain_append(ctx, &LOWERTHEN_OPENERS, mark_index);
        return;
    }

    /* Otherwise we are potential closer and we try to resolve with since all
     * the chained unresolved openers. */
    opener_index = LOWERTHEN_OPENERS.head;
    while(opener_index >= 0) {
        MD_MARK* opener = &ctx->marks[opener_index];
        OFF detected_end;
        int is_autolink = 0;
        int is_missing_mailto = 0;
        int is_raw_html = 0;

        is_autolink = (md_is_autolink(ctx, opener->beg, mark->end, &is_missing_mailto));

        if(is_autolink) {
            if(is_missing_mailto)
                opener->ch = _T('@');
        } else {
            /* Identify the line where the opening mark lives. */
            int line_index = 0;
            while(1) {
                if(opener->beg < lines[line_index].end)
                    break;
                line_index++;
            }

            is_raw_html = (md_is_html_any(ctx, lines + line_index,
                    n_lines - line_index, opener->beg, mark->end, &detected_end));
        }

        /* Check whether the range forms a valid raw HTML. */
        if(is_autolink || is_raw_html) {
            md_rollback(ctx, opener_index, mark_index, MD_ROLLBACK_ALL);
            md_resolve_range(ctx, &LOWERTHEN_OPENERS, opener_index, mark_index);

            if(is_raw_html) {
                /* If this fails, it means we have missed some earlier opportunity
                 * to resolve the opener of raw HTML. */
                MD_ASSERT(detected_end == mark->end);

                /* Make these marks zero width so the '<' and '>' are part of its
                 * contents. */
                opener->end = opener->beg;
                mark->beg = mark->end;

                opener->flags &= ~MD_MARK_AUTOLINK;
                mark->flags &= ~MD_MARK_AUTOLINK;
            } else {
                opener->flags |= MD_MARK_AUTOLINK;
                mark->flags |= MD_MARK_AUTOLINK;
            }

            /* And we are done. */
            return;
        }

        opener_index = opener->next;
    }
}

static void
md_analyze_bracket(MD_CTX* ctx, int mark_index)
{
    /* We cannot really resolve links here as for that we would need
     * more context. E.g. a following pair of brackets (reference link),
     * or enclosing pair of brackets (if the inner is the link, the outer
     * one cannot be.)
     *
     * Therefore we here only construct a list of resolved '[' ']' pairs
     * ordered by position of the closer. This allows ur to analyze what is
     * or is not link in the right order, from inside to outside in case
     * of nested brackets.
     *
     * The resolving itself is deferred into md_resolve_links().
     */

    MD_MARK* mark = &ctx->marks[mark_index];

    if(mark->flags & MD_MARK_POTENTIAL_OPENER) {
        md_mark_chain_append(ctx, &BRACKET_OPENERS, mark_index);
        return;
    }

    if(BRACKET_OPENERS.tail >= 0) {
        /* Pop the opener from the chain. */
        int opener_index = BRACKET_OPENERS.tail;
        MD_MARK* opener = &ctx->marks[opener_index];
        if(opener->prev >= 0)
            ctx->marks[opener->prev].next = -1;
        else
            BRACKET_OPENERS.head = -1;
        BRACKET_OPENERS.tail = opener->prev;

        /* Interconnect the opener and closer. */
        opener->next = mark_index;
        mark->prev = opener_index;

        /* Add the pair into chain of potential links for md_resolve_links().
         * Note we misuse opener->prev for this as opener->next points to its
         * closer. */
        if(ctx->unresolved_link_tail >= 0)
            ctx->marks[ctx->unresolved_link_tail].prev = opener_index;
        else
            ctx->unresolved_link_head = opener_index;
        ctx->unresolved_link_tail = opener_index;
        opener->prev = -1;
    }
}

/* Forward declaration. */
static void md_analyze_link_contents(MD_CTX* ctx, const MD_LINE* lines, int n_lines,
                                     int mark_beg, int mark_end);

static int
md_resolve_links(MD_CTX* ctx, const MD_LINE* lines, int n_lines)
{
    int opener_index = ctx->unresolved_link_head;
    OFF last_link_beg = 0;
    OFF last_link_end = 0;
    OFF last_img_beg = 0;
    OFF last_img_end = 0;

    while(opener_index >= 0) {
        MD_MARK* opener = &ctx->marks[opener_index];
        int closer_index = opener->next;
        MD_MARK* closer = &ctx->marks[closer_index];
        int next_index = opener->prev;
        MD_MARK* next_opener;
        MD_MARK* next_closer;
        MD_LINK_ATTR attr;
        int is_link = FALSE;

        if(next_index >= 0) {
            next_opener = &ctx->marks[next_index];
            next_closer = &ctx->marks[next_opener->next];
        } else {
            next_opener = NULL;
            next_closer = NULL;
        }

        /* If nested ("[ [ ] ]"), we need to make sure that:
         *   - The outer does not end inside of (...) belonging to the inner.
         *   - The outer cannot be link if the inner is link (i.e. not image).
         *
         * (Note we here analyze from inner to outer as the marks are ordered
         * by closer->beg.)
         */
        if((opener->beg < last_link_beg  &&  closer->end < last_link_end)  ||
           (opener->beg < last_img_beg  &&  closer->end < last_img_end)  ||
           (opener->beg < last_link_end  &&  opener->ch == '['))
        {
            opener_index = next_index;
            continue;
        }

        if(next_opener != NULL  &&  next_opener->beg == closer->end) {
            if(next_closer->beg > closer->end + 1) {
                /* Might be full reference link. */
                is_link = md_is_link_reference(ctx, lines, n_lines, next_opener->beg, next_closer->end, &attr);
            } else {
                /* Might be shortcut reference link. */
                is_link = md_is_link_reference(ctx, lines, n_lines, opener->beg, closer->end, &attr);
            }

            if(is_link < 0)
                return -1;

            if(is_link) {
                /* Eat the 2nd "[...]". */
                closer->end = next_closer->end;
            }
        } else {
            if(closer->end < ctx->size  &&  CH(closer->end) == _T('(')) {
                /* Might be inline link. */
                OFF inline_link_end = UINT_MAX;

                is_link = md_is_inline_link_spec(ctx, lines, n_lines, closer->end, &inline_link_end, &attr);
                if(is_link < 0)
                    return -1;

                /* Check the closing ')' is not inside an already resolved range
                 * (i.e. a range with a higher priority), e.g. a code span. */
                if(is_link) {
                    int i = closer_index + 1;

                    while(i < ctx->n_marks) {
                        MD_MARK* mark = &ctx->marks[i];

                        if(mark->beg >= inline_link_end)
                            break;
                        if((mark->flags & (MD_MARK_OPENER | MD_MARK_RESOLVED)) == (MD_MARK_OPENER | MD_MARK_RESOLVED)) {
                            if(ctx->marks[mark->next].beg >= inline_link_end) {
                                /* Cancel the link status. */
                                if(attr.title_needs_free)
                                    free(attr.title);
                                is_link = FALSE;
                                break;
                            }

                            i = mark->next + 1;
                        } else {
                            i++;
                        }
                    }
                }

                if(is_link) {
                    /* Eat the "(...)" */
                    closer->end = inline_link_end;
                }
            }

            if(!is_link) {
                /* Might be collapsed reference link. */
                is_link = md_is_link_reference(ctx, lines, n_lines, opener->beg, closer->end, &attr);
                if(is_link < 0)
                    return -1;
            }
        }

        if(is_link) {
            /* Resolve the brackets as a link. */
            opener->flags |= MD_MARK_OPENER | MD_MARK_RESOLVED;
            closer->flags |= MD_MARK_CLOSER | MD_MARK_RESOLVED;

            /* If it is a link, we store the destination and title in the two
             * dummy marks after the opener. */
            MD_ASSERT(ctx->marks[opener_index+1].ch == 'D');
            ctx->marks[opener_index+1].beg = attr.dest_beg;
            ctx->marks[opener_index+1].end = attr.dest_end;

            MD_ASSERT(ctx->marks[opener_index+2].ch == 'D');
            md_mark_store_ptr(ctx, opener_index+2, attr.title);
            if(attr.title_needs_free)
                md_mark_chain_append(ctx, &PTR_CHAIN, opener_index+2);
            ctx->marks[opener_index+2].prev = attr.title_size;

            if(opener->ch == '[') {
                last_link_beg = opener->beg;
                last_link_end = closer->end;
            } else {
                last_img_beg = opener->beg;
                last_img_end = closer->end;
            }

            md_analyze_link_contents(ctx, lines, n_lines, opener_index+1, closer_index);
        }

        opener_index = next_index;
    }

    return 0;
}

/* Analyze whether the mark '&' starts a HTML entity.
 * If so, update its flags as well as flags of corresponding closer ';'. */
static void
md_analyze_entity(MD_CTX* ctx, int mark_index)
{
    MD_MARK* opener = &ctx->marks[mark_index];
    MD_MARK* closer;
    OFF off;

    /* Cannot be entity if there is no closer as the next mark.
     * (Any other mark between would mean strange character which cannot be
     * part of the entity.
     *
     * So we can do all the work on '&' and do not call this later for the
     * closing mark ';'.
     */
    if(mark_index + 1 >= ctx->n_marks)
        return;
    closer = &ctx->marks[mark_index+1];
    if(closer->ch != ';')
        return;

    if(md_is_entity(ctx, opener->beg, closer->end, &off)) {
        MD_ASSERT(off == closer->end);

        md_resolve_range(ctx, NULL, mark_index, mark_index+1);
        opener->end = closer->end;
    }
}

static void
md_analyze_table_cell_boundary(MD_CTX* ctx, int mark_index)
{
    MD_MARK* mark = &ctx->marks[mark_index];
    mark->flags |= MD_MARK_RESOLVED;

    md_mark_chain_append(ctx, &TABLECELLBOUNDARIES, mark_index);
    ctx->n_table_cell_boundaries++;
}

/* Split a longer mark into two. The new mark takes the given count of
 * characters. May only be called if an adequate number of dummy 'D' marks
 * follows.
 */
static int
md_split_simple_pairing_mark(MD_CTX* ctx, int mark_index, SZ n)
{
    MD_MARK* mark = &ctx->marks[mark_index];
    int new_mark_index = mark_index + (mark->end - mark->beg - n);
    MD_MARK* dummy = &ctx->marks[new_mark_index];

    MD_ASSERT(mark->end - mark->beg > n);
    MD_ASSERT(dummy->ch == 'D');

    memcpy(dummy, mark, sizeof(MD_MARK));
    mark->end -= n;
    dummy->beg = mark->end;

    return new_mark_index;
}

static void
md_analyze_simple_pairing_mark(MD_CTX* ctx, MD_MARKCHAIN* chain, int mark_index,
                               int apply_rule_of_three)
{
    MD_MARK* mark = &ctx->marks[mark_index];

    /* If we can be a closer, try to resolve with the preceding opener. */
    if((mark->flags & MD_MARK_POTENTIAL_CLOSER)  &&  chain->tail >= 0) {
        int opener_index = chain->tail;
        MD_MARK* opener = &ctx->marks[opener_index];
        SZ opener_size = opener->end - opener->beg;
        SZ closer_size = mark->end - mark->beg;

        /* Apply the "rule of three". */
        if(apply_rule_of_three) {
            while((mark->flags & MD_MARK_EMPH_INTRAWORD) || (opener->flags & MD_MARK_EMPH_INTRAWORD)) {
                SZ opener_orig_size_modulo3;

                switch(opener->flags & MD_MARK_EMPH_MODULO3_MASK) {
                    case MD_MARK_EMPH_MODULO3_0:    opener_orig_size_modulo3 = 0; break;
                    case MD_MARK_EMPH_MODULO3_1:    opener_orig_size_modulo3 = 1; break;
                    case MD_MARK_EMPH_MODULO3_2:    opener_orig_size_modulo3 = 2; break;
                    default:                        MD_UNREACHABLE(); break;
                }

                if((opener_orig_size_modulo3 + closer_size) % 3 != 0) {
                    /* This opener is suitable. */
                    break;
                }

                if(opener->prev >= 0) {
                    /* Try previous opener. */
                    opener_index = opener->prev;
                    opener = &ctx->marks[opener_index];
                    opener_size = opener->end - opener->beg;
                    closer_size = mark->end - mark->beg;
                } else {
                    /* No suitable opener found. */
                    goto cannot_resolve;
                }
            }
        }

        if(opener_size > closer_size) {
            opener_index = md_split_simple_pairing_mark(ctx, opener_index, closer_size);
            md_mark_chain_append(ctx, chain, opener_index);
        } else if(opener_size < closer_size) {
            md_split_simple_pairing_mark(ctx, mark_index, closer_size - opener_size);
        }

        md_rollback(ctx, opener_index, mark_index, MD_ROLLBACK_CROSSING);
        md_resolve_range(ctx, chain, opener_index, mark_index);
        return;
    }

cannot_resolve:
    /* If not resolved, and we can be an opener, remember the mark for
     * the future. */
    if(mark->flags & MD_MARK_POTENTIAL_OPENER)
        md_mark_chain_append(ctx, chain, mark_index);
}

static inline void
md_analyze_asterisk(MD_CTX* ctx, int mark_index)
{
    md_analyze_simple_pairing_mark(ctx, &ASTERISK_OPENERS, mark_index, 1);
}

static inline void
md_analyze_underscore(MD_CTX* ctx, int mark_index)
{
    md_analyze_simple_pairing_mark(ctx, &UNDERSCORE_OPENERS, mark_index, 1);
}

static void
md_analyze_tilde(MD_CTX* ctx, int mark_index)
{
    /* We attempt to be Github Flavored Markdown compatible here. GFM says
     * that length of the tilde sequence is not important at all. Note that
     * implies the TILDE_OPENERS chain can have at most one item. */

    if(TILDE_OPENERS.head >= 0) {
        /* The chain already contains an opener, so we may resolve the span. */
        int opener_index = TILDE_OPENERS.head;

        md_rollback(ctx, opener_index, mark_index, MD_ROLLBACK_CROSSING);
        md_resolve_range(ctx, &TILDE_OPENERS, opener_index, mark_index);
    } else {
        /* We can only be opener. */
        md_mark_chain_append(ctx, &TILDE_OPENERS, mark_index);
    }
}

static void
md_analyze_permissive_url_autolink(MD_CTX* ctx, int mark_index)
{
    MD_MARK* opener = &ctx->marks[mark_index];
    int closer_index = mark_index + 1;
    MD_MARK* closer = &ctx->marks[closer_index];
    MD_MARK* next_resolved_mark;
    OFF off = opener->end;
    int seen_dot = FALSE;
    int seen_underscore_or_hyphen[2] = { FALSE, FALSE };

    /* Check for domain. */
    while(off < ctx->size) {
        if(ISALNUM(off)) {
            off++;
        } else if(CH(off) == _T('.')) {
            seen_dot = TRUE;
            seen_underscore_or_hyphen[0] = seen_underscore_or_hyphen[1];
            seen_underscore_or_hyphen[1] = FALSE;
            off++;
        } else if(ISANYOF2(off, _T('-'), _T('_'))) {
            seen_underscore_or_hyphen[1] = TRUE;
            off++;
        } else {
            break;
        }
    }

    if(off <= opener->end || !seen_dot || seen_underscore_or_hyphen[0] || seen_underscore_or_hyphen[1])
        return;

    /* Check for path. */
    next_resolved_mark = closer + 1;
    while(next_resolved_mark->ch == 'D' || !(next_resolved_mark->flags & MD_MARK_RESOLVED))
        next_resolved_mark++;
    while(off < next_resolved_mark->beg  &&  CH(off) != _T('<')  &&  !ISWHITESPACE(off)  &&  !ISNEWLINE(off))
        off++;

    /* Path validation. */
    if(ISANYOF(off-1, _T("?!.,:*_~)"))) {
        if(CH(off-1) != _T(')')) {
            off--;
        } else {
            int parenthesis_balance = 0;
            OFF tmp;

            for(tmp = opener->end; tmp < off; tmp++) {
                if(CH(tmp) == _T('('))
                    parenthesis_balance++;
                else if(CH(tmp) == _T(')'))
                    parenthesis_balance--;
            }

            if(parenthesis_balance < 0)
                off--;
        }
    }

    /* Ok. Lets call it auto-link. Adapt opener and create closer to zero
     * length so all the contents becomes the link text. */
    MD_ASSERT(closer->ch == 'D');
    opener->end = opener->beg;
    closer->ch = opener->ch;
    closer->beg = off;
    closer->end = off;
    md_resolve_range(ctx, NULL, mark_index, closer_index);
}

/* The permissive autolinks do not have to be enclosed in '<' '>' but we
 * instead impose stricter rules what is understood as an e-mail address
 * here. Actually any non-alphanumeric characters with exception of '.'
 * are prohibited both in username and after '@'. */
static void
md_analyze_permissive_email_autolink(MD_CTX* ctx, int mark_index)
{
    MD_MARK* opener = &ctx->marks[mark_index];
    int closer_index;
    MD_MARK* closer;
    OFF beg = opener->beg;
    OFF end = opener->end;
    int dot_count = 0;

    MD_ASSERT(CH(beg) == _T('@'));

    /* Scan for name before '@'. */
    while(beg > 0  &&  (ISALNUM(beg-1) || ISANYOF(beg-1, _T(".-_+"))))
        beg--;

    /* Scan for domain after '@'. */
    while(end < ctx->size  &&  (ISALNUM(end) || ISANYOF(end, _T(".-_")))) {
        if(CH(end) == _T('.'))
            dot_count++;
        end++;
    }
    if(CH(end-1) == _T('.')) {  /* Final '.' not part of it. */
        dot_count--;
        end--;
    }
    else if(ISANYOF2(end-1, _T('-'), _T('_'))) /* These are forbidden at the end. */
        return;
    if(CH(end-1) == _T('@')  ||  dot_count == 0)
        return;

    /* Ok. Lets call it auto-link. Adapt opener and create closer to zero
     * length so all the contents becomes the link text. */
    closer_index = mark_index + 1;
    closer = &ctx->marks[closer_index];
    MD_ASSERT(closer->ch == 'D');

    opener->beg = beg;
    opener->end = beg;
    closer->ch = opener->ch;
    closer->beg = end;
    closer->end = end;
    md_resolve_range(ctx, NULL, mark_index, closer_index);
}

static inline void
md_analyze_marks(MD_CTX* ctx, const MD_LINE* lines, int n_lines,
                 int mark_beg, int mark_end, const CHAR* mark_chars)
{
    int i = mark_beg;

    while(i < mark_end) {
        MD_MARK* mark = &ctx->marks[i];

        /* Skip resolved spans. */
        if(mark->flags & MD_MARK_RESOLVED) {
            if(mark->flags & MD_MARK_OPENER) {
                MD_ASSERT(i < mark->next);
                i = mark->next + 1;
            } else {
                i++;
            }
            continue;
        }

        /* Skip marks we do not want to deal with. */
        if(!ISANYOF_(mark->ch, mark_chars)) {
            i++;
            continue;
        }

        /* Analyze the mark. */
        switch(mark->ch) {
            case '`':   md_analyze_backtick(ctx, i); break;
            case '<':   /* Pass through. */
            case '>':   md_analyze_lt_gt(ctx, i, lines, n_lines); break;
            case '[':   /* Pass through. */
            case '!':   /* Pass through. */
            case ']':   md_analyze_bracket(ctx, i); break;
            case '&':   md_analyze_entity(ctx, i); break;
            case '|':   md_analyze_table_cell_boundary(ctx, i); break;
            case '*':   md_analyze_asterisk(ctx, i); break;
            case '_':   md_analyze_underscore(ctx, i); break;
            case '~':   md_analyze_tilde(ctx, i); break;
            case '.':   /* Pass through. */
            case ':':   md_analyze_permissive_url_autolink(ctx, i); break;
            case '@':   md_analyze_permissive_email_autolink(ctx, i); break;
        }

        i++;
    }
}

/* Analyze marks (build ctx->marks). */
static int
md_analyze_inlines(MD_CTX* ctx, const MD_LINE* lines, int n_lines, int table_mode)
{
    int ret;

    /* Reset the previously collected stack of marks. */
    ctx->n_marks = 0;

    /* Collect all marks. */
    MD_CHECK(md_collect_marks(ctx, lines, n_lines, table_mode));

    /* We analyze marks in few groups to handle their precedence. */
    /* (1) Entities; code spans; autolinks; raw HTML. */
    md_analyze_marks(ctx, lines, n_lines, 0, ctx->n_marks, _T("&`<>"));
    BACKTICK_OPENERS.head = -1;
    BACKTICK_OPENERS.tail = -1;
    LOWERTHEN_OPENERS.head = -1;
    LOWERTHEN_OPENERS.tail = -1;

    if(table_mode) {
        /* (2) Analyze table cell boundaries.
         * Note we reset TABLECELLBOUNDARIES chain prior to the call md_analyze_marks(),
         * not after, because caller may need it. */
        MD_ASSERT(n_lines == 1);
        TABLECELLBOUNDARIES.head = -1;
        TABLECELLBOUNDARIES.tail = -1;
        ctx->n_table_cell_boundaries = 0;
        md_analyze_marks(ctx, lines, n_lines, 0, ctx->n_marks, _T("|"));
        return ret;
    }

    /* (3) Links. */
    md_analyze_marks(ctx, lines, n_lines, 0, ctx->n_marks, _T("[]!"));
    MD_CHECK(md_resolve_links(ctx, lines, n_lines));
    BRACKET_OPENERS.head = -1;
    BRACKET_OPENERS.tail = -1;
    ctx->unresolved_link_head = -1;
    ctx->unresolved_link_tail = -1;

    /* (4) Emphasis and strong emphasis; permissive autolinks. */
    md_analyze_link_contents(ctx, lines, n_lines, 0, ctx->n_marks);

abort:
    return ret;
}

static void
md_analyze_link_contents(MD_CTX* ctx, const MD_LINE* lines, int n_lines,
                         int mark_beg, int mark_end)
{
    md_analyze_marks(ctx, lines, n_lines, mark_beg, mark_end, _T("*_~@:."));
    ASTERISK_OPENERS.head = -1;
    ASTERISK_OPENERS.tail = -1;
    UNDERSCORE_OPENERS.head = -1;
    UNDERSCORE_OPENERS.tail = -1;
    TILDE_OPENERS.head = -1;
    TILDE_OPENERS.tail = -1;
}

static int
md_enter_leave_span_a(MD_CTX* ctx, int enter, MD_SPANTYPE type,
                      const CHAR* dest, SZ dest_size, int prohibit_escapes_in_dest,
                      const CHAR* title, SZ title_size)
{
    MD_ATTRIBUTE_BUILD href_build = { 0 };
    MD_ATTRIBUTE_BUILD title_build = { 0 };
    MD_SPAN_A_DETAIL det;
    int ret = 0;

    /* Note we here rely on fact that MD_SPAN_A_DETAIL and
     * MD_SPAN_IMG_DETAIL are binary-compatible. */
    memset(&det, 0, sizeof(MD_SPAN_A_DETAIL));
    MD_CHECK(md_build_attribute(ctx, dest, dest_size,
                    (prohibit_escapes_in_dest ? MD_BUILD_ATTR_NO_ESCAPES : 0),
                    &det.href, &href_build));
    MD_CHECK(md_build_attribute(ctx, title, title_size, 0, &det.title, &title_build));

    if(enter)
        MD_ENTER_SPAN(type, &det);
    else
        MD_LEAVE_SPAN(type, &det);

abort:
    md_free_attribute(ctx, &href_build);
    md_free_attribute(ctx, &title_build);
    return ret;
}

/* Render the output, accordingly to the analyzed ctx->marks. */
static int
md_process_inlines(MD_CTX* ctx, const MD_LINE* lines, int n_lines)
{
    MD_TEXTTYPE text_type;
    const MD_LINE* line = lines;
    MD_MARK* prev_mark = NULL;
    MD_MARK* mark;
    OFF off = lines[0].beg;
    OFF end = lines[n_lines-1].end;
    int enforce_hardbreak = 0;
    int ret = 0;

    /* Find first resolved mark. Note there is always at least one resolved
     * mark,  the dummy last one after the end of the latest line we actually
     * never really reach. This saves us of a lot of special checks and cases
     * in this function. */
    mark = ctx->marks;
    while(!(mark->flags & MD_MARK_RESOLVED))
        mark++;

    text_type = MD_TEXT_NORMAL;

    while(1) {
        /* Process the text up to the next mark or end-of-line. */
        OFF tmp = (line->end < mark->beg ? line->end : mark->beg);
        if(tmp > off) {
            MD_TEXT(text_type, STR(off), tmp - off);
            off = tmp;
        }

        /* If reached the mark, process it and move to next one. */
        if(off >= mark->beg) {
            switch(mark->ch) {
                case '\\':      /* Backslash escape. */
                    if(ISNEWLINE(mark->beg+1))
                        enforce_hardbreak = 1;
                    else
                        MD_TEXT(text_type, STR(mark->beg+1), 1);
                    break;

                case ' ':       /* Non-trivial space. */
                    MD_TEXT(text_type, _T(" "), 1);
                    break;

                case '`':       /* Code span. */
                    if(mark->flags & MD_MARK_OPENER) {
                        MD_ENTER_SPAN(MD_SPAN_CODE, NULL);
                        text_type = MD_TEXT_CODE;
                    } else {
                        MD_LEAVE_SPAN(MD_SPAN_CODE, NULL);
                        text_type = MD_TEXT_NORMAL;
                    }
                    break;

                case '_':
                case '*':       /* Emphasis, strong emphasis. */
                    if(mark->flags & MD_MARK_OPENER) {
                        if((mark->end - off) % 2) {
                            MD_ENTER_SPAN(MD_SPAN_EM, NULL);
                            off++;
                        }
                        while(off + 1 < mark->end) {
                            MD_ENTER_SPAN(MD_SPAN_STRONG, NULL);
                            off += 2;
                        }
                    } else {
                        while(off + 1 < mark->end) {
                            MD_LEAVE_SPAN(MD_SPAN_STRONG, NULL);
                            off += 2;
                        }
                        if((mark->end - off) % 2) {
                            MD_LEAVE_SPAN(MD_SPAN_EM, NULL);
                            off++;
                        }
                    }
                    break;

                case '~':
                    if(mark->flags & MD_MARK_OPENER)
                        MD_ENTER_SPAN(MD_SPAN_DEL, NULL);
                    else
                        MD_LEAVE_SPAN(MD_SPAN_DEL, NULL);
                    break;

                case '[':       /* Link, image. */
                case '!':
                case ']':
                {
                    const MD_MARK* opener = (mark->ch != ']' ? mark : &ctx->marks[mark->prev]);
                    const MD_MARK* dest_mark = opener+1;
                    const MD_MARK* title_mark = opener+2;

                    MD_ASSERT(dest_mark->ch == 'D');
                    MD_ASSERT(title_mark->ch == 'D');

                    MD_CHECK(md_enter_leave_span_a(ctx, (mark->ch != ']'),
                                (opener->ch == '!' ? MD_SPAN_IMG : MD_SPAN_A),
                                STR(dest_mark->beg), dest_mark->end - dest_mark->beg, FALSE,
                                md_mark_get_ptr(ctx, title_mark - ctx->marks), title_mark->prev));

                    /* link/image closer may span multiple lines. */
                    if(mark->ch == ']') {
                        while(mark->end > line->end)
                            line++;
                    }

                    break;
                }

                case '<':
                case '>':       /* Autolink or raw HTML. */
                    if(!(mark->flags & MD_MARK_AUTOLINK)) {
                        /* Raw HTML. */
                        if(mark->flags & MD_MARK_OPENER)
                            text_type = MD_TEXT_HTML;
                        else
                            text_type = MD_TEXT_NORMAL;
                        break;
                    }
                    /* Pass through, if auto-link. */

                case '@':       /* Permissive e-mail autolink. */
                case ':':       /* Permissive URL autolink. */
                case '.':       /* Permissive WWW autolink. */
                {
                    MD_MARK* opener = ((mark->flags & MD_MARK_OPENER) ? mark : &ctx->marks[mark->prev]);
                    MD_MARK* closer = &ctx->marks[opener->next];
                    const CHAR* dest = STR(opener->end);
                    SZ dest_size = closer->beg - opener->end;

                    /* For permissive auto-links we do not know closer mark
                     * position at the time of md_collect_marks(), therefore
                     * it can be out-of-order in ctx->marks[].
                     *
                     * With this flag, we make sure that we output the closer
                     * only if we processed the opener. */
                    if(mark->flags & MD_MARK_OPENER)
                        closer->flags |= MD_MARK_VALIDPERMISSIVEAUTOLINK;

                    if(opener->ch == '@' || opener->ch == '.') {
                        dest_size += 7;
                        MD_TEMP_BUFFER(dest_size * sizeof(CHAR));
                        memcpy(ctx->buffer,
                                (opener->ch == '@' ? _T("mailto:") : _T("http://")),
                                7 * sizeof(CHAR));
                        memcpy(ctx->buffer + 7, dest, (dest_size-7) * sizeof(CHAR));
                        dest = ctx->buffer;
                    }

                    if(closer->flags & MD_MARK_VALIDPERMISSIVEAUTOLINK)
                        MD_CHECK(md_enter_leave_span_a(ctx, (mark->flags & MD_MARK_OPENER),
                                    MD_SPAN_A, dest, dest_size, TRUE, NULL, 0));
                    break;
                }

                case '&':       /* Entity. */
                    MD_TEXT(MD_TEXT_ENTITY, STR(mark->beg), mark->end - mark->beg);
                    break;

                case '\0':
                    MD_TEXT(MD_TEXT_NULLCHAR, _T(""), 1);
                    break;

                case 127:
                    goto abort;
            }

            off = mark->end;

            /* Move to next resolved mark. */
            prev_mark = mark;
            mark++;
            while(!(mark->flags & MD_MARK_RESOLVED)  ||  mark->beg < off)
                mark++;
        }

        /* If reached end of line, move to next one. */
        if(off >= line->end) {
            /* If it is the last line, we are done. */
            if(off >= end)
                break;

            if(text_type == MD_TEXT_CODE) {
                /* Inside code spans, new lines are transformed into single
                 * spaces. */
                MD_ASSERT(prev_mark != NULL);
                MD_ASSERT(prev_mark->ch == '`'  &&  (prev_mark->flags & MD_MARK_OPENER));
                MD_ASSERT(mark->ch == '`'  &&  (mark->flags & MD_MARK_CLOSER));

                if(prev_mark->end < off  &&  off < mark->beg)
                    MD_TEXT(MD_TEXT_CODE, _T(" "), 1);
            } else if(text_type == MD_TEXT_HTML) {
                /* Inside raw HTML, we output the new line verbatim, including
                 * any trailing spaces. */
                OFF tmp = off;

                while(tmp < end  &&  ISBLANK(tmp))
                    tmp++;
                if(tmp > off)
                    MD_TEXT(MD_TEXT_HTML, STR(off), tmp - off);
                MD_TEXT(MD_TEXT_HTML, _T("\n"), 1);
            } else {
                /* Output soft or hard line break. */
                MD_TEXTTYPE break_type = MD_TEXT_SOFTBR;

                if(text_type == MD_TEXT_NORMAL) {
                    if(enforce_hardbreak)
                        break_type = MD_TEXT_BR;
                    else if((CH(line->end) == _T(' ') && CH(line->end+1) == _T(' ')))
                        break_type = MD_TEXT_BR;
                }

                MD_TEXT(break_type, _T("\n"), 1);
            }

            /* Move to the next line. */
            line++;
            off = line->beg;

            enforce_hardbreak = 0;
        }
    }

abort:
    return ret;
}


/***************************
 ***  Processing Tables  ***
 ***************************/

static void
md_analyze_table_alignment(MD_CTX* ctx, OFF beg, OFF end, MD_ALIGN* align, int n_align)
{
    static const MD_ALIGN align_map[] = { MD_ALIGN_DEFAULT, MD_ALIGN_LEFT, MD_ALIGN_RIGHT, MD_ALIGN_CENTER };
    OFF off = beg;

    while(n_align > 0) {
        int index = 0;  /* index into align_map[] */

        while(CH(off) != _T('-'))
            off++;
        if(off > beg  &&  CH(off-1) == _T(':'))
            index |= 1;
        while(off < end  &&  CH(off) == _T('-'))
            off++;
        if(off < end  &&  CH(off) == _T(':'))
            index |= 2;

        *align = align_map[index];
        align++;
        n_align--;
    }

}

/* Forward declaration. */
static int md_process_normal_block_contents(MD_CTX* ctx, const MD_LINE* lines, int n_lines);

static int
md_process_table_cell(MD_CTX* ctx, MD_BLOCKTYPE cell_type, MD_ALIGN align, OFF beg, OFF end)
{
    MD_LINE line;
    MD_BLOCK_TD_DETAIL det;
    int ret = 0;

    while(beg < end  &&  ISWHITESPACE(beg))
        beg++;
    while(end > beg  &&  ISWHITESPACE(end-1))
        end--;

    det.align = align;
    line.beg = beg;
    line.end = end;

    MD_ENTER_BLOCK(cell_type, &det);
    MD_CHECK(md_process_normal_block_contents(ctx, &line, 1));
    MD_LEAVE_BLOCK(cell_type, &det);

abort:
    return ret;
}

static int
md_process_table_row(MD_CTX* ctx, MD_BLOCKTYPE cell_type, OFF beg, OFF end,
                     const MD_ALIGN* align, int col_count)
{
    MD_LINE line;
    OFF* pipe_offs = NULL;
    int i, j, n;
    int ret = 0;

    line.beg = beg;
    line.end = end;

    /* Break the line into table cells by identifying pipe characters who
     * form the cell boundary. */
    MD_CHECK(md_analyze_inlines(ctx, &line, 1, TRUE));

    /* We have to remember the cell boundaries in local buffer because
     * ctx->marks[] shall be reused during cell contents processing. */
    n = ctx->n_table_cell_boundaries;
    pipe_offs = (OFF*) malloc(n * sizeof(OFF));
    if(pipe_offs == NULL) {
        MD_LOG("malloc() failed.");
        ret = -1;
        goto abort;
    }
    for(i = TABLECELLBOUNDARIES.head, j = 0; i >= 0; i = ctx->marks[i].next) {
        MD_MARK* mark = &ctx->marks[i];
        pipe_offs[j++] = mark->beg;
    }

    /* Process cells. */
    MD_ENTER_BLOCK(MD_BLOCK_TR, NULL);
    j = 0;
    if(beg < pipe_offs[0]  &&  j < col_count)
        MD_CHECK(md_process_table_cell(ctx, cell_type, align[j++], beg, pipe_offs[0]));
    for(i = 0; i < n-1  &&  j < col_count; i++)
        MD_CHECK(md_process_table_cell(ctx, cell_type, align[j++], pipe_offs[i]+1, pipe_offs[i+1]));
    if(pipe_offs[n-1] < end-1  &&  j < col_count)
        MD_CHECK(md_process_table_cell(ctx, cell_type, align[j++], pipe_offs[n-1]+1, end));
    /* Make sure we call enough table cells even if the current table contains
     * too few of them. */
    while(j < col_count)
        MD_CHECK(md_process_table_cell(ctx, cell_type, align[j++], 0, 0));

    MD_LEAVE_BLOCK(MD_BLOCK_TR, NULL);

abort:
    free(pipe_offs);

    /* Free any temporary memory blocks stored within some dummy marks. */
    for(i = PTR_CHAIN.head; i >= 0; i = ctx->marks[i].next)
        free(md_mark_get_ptr(ctx, i));
    PTR_CHAIN.head = -1;
    PTR_CHAIN.tail = -1;

    return ret;
}

static int
md_process_table_block_contents(MD_CTX* ctx, int col_count, const MD_LINE* lines, int n_lines)
{
    MD_ALIGN* align;
    int i;
    int ret = 0;

    /* At least two lines have to be present: The column headers and the line
     * with the underlines. */
    MD_ASSERT(n_lines >= 2);

    align = malloc(col_count * sizeof(MD_ALIGN));
    if(align == NULL) {
        MD_LOG("malloc() failed.");
        ret = -1;
        goto abort;
    }

    md_analyze_table_alignment(ctx, lines[1].beg, lines[1].end, align, col_count);

    MD_ENTER_BLOCK(MD_BLOCK_THEAD, NULL);
    MD_CHECK(md_process_table_row(ctx, MD_BLOCK_TH,
                        lines[0].beg, lines[0].end, align, col_count));
    MD_LEAVE_BLOCK(MD_BLOCK_THEAD, NULL);

    MD_ENTER_BLOCK(MD_BLOCK_TBODY, NULL);
    for(i = 2; i < n_lines; i++) {
        MD_CHECK(md_process_table_row(ctx, MD_BLOCK_TD,
                        lines[i].beg, lines[i].end, align, col_count));
    }
    MD_LEAVE_BLOCK(MD_BLOCK_TBODY, NULL);

abort:
    free(align);
    return ret;
}

static int
md_is_table_row(MD_CTX* ctx, OFF beg, OFF* p_end)
{
    MD_LINE line;
    int i;
    int ret = FALSE;

    line.beg = beg;
    line.end = beg;

    /* Find end of line. */
    while(line.end < ctx->size  &&  !ISNEWLINE(line.end))
        line.end++;

    MD_CHECK(md_analyze_inlines(ctx, &line, 1, TRUE));

    if(TABLECELLBOUNDARIES.head >= 0) {
        if(p_end != NULL)
            *p_end = line.end;
        ret = TRUE;
    }

abort:
    /* Free any temporary memory blocks stored within some dummy marks. */
    for(i = PTR_CHAIN.head; i >= 0; i = ctx->marks[i].next)
        free(md_mark_get_ptr(ctx, i));
    PTR_CHAIN.head = -1;
    PTR_CHAIN.tail = -1;

    return ret;
}


/**************************
 ***  Processing Block  ***
 **************************/

#define MD_BLOCK_CONTAINER_OPENER   0x01
#define MD_BLOCK_CONTAINER_CLOSER   0x02
#define MD_BLOCK_CONTAINER          (MD_BLOCK_CONTAINER_OPENER | MD_BLOCK_CONTAINER_CLOSER)
#define MD_BLOCK_LOOSE_LIST         0x04

struct MD_BLOCK_tag {
    MD_BLOCKTYPE type  :  8;
    unsigned flags     :  8;

    /* MD_BLOCK_H:      Header level (1 - 6)
     * MD_BLOCK_CODE:   Non-zero if fenced, zero if indented.
     * MD_BLOCK_LI:     Task mark character (0 if not task list item, 'x', 'X' or ' ').
     * MD_BLOCK_TABLE:  Column count (as determined by the table underline).
     */
    unsigned data      : 16;

    /* Leaf blocks:     Count of lines (MD_LINE or MD_VERBATIMLINE) on the block.
     * MD_BLOCK_LI:     Task mark offset in the input doc.
     * MD_BLOCK_OL:     Start item number.
     */
    unsigned n_lines;
};

struct MD_CONTAINER_tag {
    CHAR ch;
    unsigned is_loose    : 8;
    unsigned is_task     : 8;
    unsigned start;
    unsigned mark_indent;
    unsigned contents_indent;
    OFF block_byte_off;
    OFF task_mark_off;
};


static int
md_process_normal_block_contents(MD_CTX* ctx, const MD_LINE* lines, int n_lines)
{
    int i;
    int ret;

    MD_CHECK(md_analyze_inlines(ctx, lines, n_lines, FALSE));
    MD_CHECK(md_process_inlines(ctx, lines, n_lines));

abort:
    /* Free any temporary memory blocks stored within some dummy marks. */
    for(i = PTR_CHAIN.head; i >= 0; i = ctx->marks[i].next)
        free(md_mark_get_ptr(ctx, i));
    PTR_CHAIN.head = -1;
    PTR_CHAIN.tail = -1;

    return ret;
}

static int
md_process_verbatim_block_contents(MD_CTX* ctx, MD_TEXTTYPE text_type, const MD_VERBATIMLINE* lines, int n_lines)
{
    static const CHAR indent_chunk_str[] = _T("                ");
    static const SZ indent_chunk_size = SIZEOF_ARRAY(indent_chunk_str) - 1;

    int i;
    int ret = 0;

    for(i = 0; i < n_lines; i++) {
        const MD_VERBATIMLINE* line = &lines[i];
        int indent = line->indent;

        MD_ASSERT(indent >= 0);

        /* Output code indentation. */
        while(indent > SIZEOF_ARRAY(indent_chunk_str)) {
            MD_TEXT(text_type, indent_chunk_str, indent_chunk_size);
            indent -= SIZEOF_ARRAY(indent_chunk_str);
        }
        if(indent > 0)
            MD_TEXT(text_type, indent_chunk_str, indent);

        /* Output the code line itself. */
        MD_TEXT_INSECURE(text_type, STR(line->beg), line->end - line->beg);

        /* Enforce end-of-line. */
        MD_TEXT(text_type, _T("\n"), 1);
    }

abort:
    return ret;
}

static int
md_process_code_block_contents(MD_CTX* ctx, int is_fenced, const MD_VERBATIMLINE* lines, int n_lines)
{
    if(is_fenced) {
        /* Skip the first line in case of fenced code: It is the fence.
         * (Only the starting fence is present due to logic in md_analyze_line().) */
        lines++;
        n_lines--;
    } else {
        /* Ignore blank lines at start/end of indented code block. */
        while(n_lines > 0  &&  lines[0].beg == lines[0].end) {
            lines++;
            n_lines--;
        }
        while(n_lines > 0  &&  lines[n_lines-1].beg == lines[n_lines-1].end) {
            n_lines--;
        }
    }

    if(n_lines == 0)
        return 0;

    return md_process_verbatim_block_contents(ctx, MD_TEXT_CODE, lines, n_lines);
}

static int
md_setup_fenced_code_detail(MD_CTX* ctx, const MD_BLOCK* block, MD_BLOCK_CODE_DETAIL* det,
                            MD_ATTRIBUTE_BUILD* info_build, MD_ATTRIBUTE_BUILD* lang_build)
{
    const MD_VERBATIMLINE* fence_line = (const MD_VERBATIMLINE*)(block + 1);
    OFF beg = fence_line->beg;
    OFF end = fence_line->end;
    OFF lang_end;
    CHAR fence_ch = CH(fence_line->beg);
    int ret = 0;

    /* Skip the fence itself. */
    while(beg < ctx->size  &&  CH(beg) == fence_ch)
        beg++;
    /* Trim initial spaces. */
    while(beg < ctx->size  &&  CH(beg) == _T(' '))
        beg++;

    /* Trim trailing spaces. */
    while(end > beg  &&  CH(end-1) == _T(' '))
        end--;

    /* Build info string attribute. */
    MD_CHECK(md_build_attribute(ctx, STR(beg), end - beg, 0, &det->info, info_build));

    /* Build info string attribute. */
    lang_end = beg;
    while(lang_end < end  &&  !ISWHITESPACE(lang_end))
        lang_end++;
    MD_CHECK(md_build_attribute(ctx, STR(beg), lang_end - beg, 0, &det->lang, lang_build));

abort:
    return ret;
}

static int
md_process_leaf_block(MD_CTX* ctx, const MD_BLOCK* block)
{
    union {
        MD_BLOCK_H_DETAIL header;
        MD_BLOCK_CODE_DETAIL code;
    } det;
    MD_ATTRIBUTE_BUILD info_build;
    MD_ATTRIBUTE_BUILD lang_build;
    int is_in_tight_list;
    int clean_fence_code_detail = FALSE;
    int ret = 0;

    memset(&det, 0, sizeof(det));

    if(ctx->n_containers == 0)
        is_in_tight_list = FALSE;
    else
        is_in_tight_list = !ctx->containers[ctx->n_containers-1].is_loose;

    switch(block->type) {
        case MD_BLOCK_H:
            det.header.level = block->data;
            break;

        case MD_BLOCK_CODE:
            /* For fenced code block, we may need to set the info string. */
            if(block->data != 0) {
                memset(&det.code, 0, sizeof(MD_BLOCK_CODE_DETAIL));
                clean_fence_code_detail = TRUE;
                MD_CHECK(md_setup_fenced_code_detail(ctx, block, &det.code, &info_build, &lang_build));
            }
            break;

        default:
            /* Noop. */
            break;
    }

    if(!is_in_tight_list  ||  block->type != MD_BLOCK_P)
        MD_ENTER_BLOCK(block->type, (void*) &det);

    /* Process the block contents accordingly to is type. */
    switch(block->type) {
        case MD_BLOCK_HR:
            /* noop */
            break;

        case MD_BLOCK_CODE:
            MD_CHECK(md_process_code_block_contents(ctx, (block->data != 0),
                            (const MD_VERBATIMLINE*)(block + 1), block->n_lines));
            break;

        case MD_BLOCK_HTML:
            MD_CHECK(md_process_verbatim_block_contents(ctx, MD_TEXT_HTML,
                            (const MD_VERBATIMLINE*)(block + 1), block->n_lines));
            break;

        case MD_BLOCK_TABLE:
            MD_CHECK(md_process_table_block_contents(ctx, block->data,
                            (const MD_LINE*)(block + 1), block->n_lines));
            break;

        default:
            MD_CHECK(md_process_normal_block_contents(ctx,
                            (const MD_LINE*)(block + 1), block->n_lines));
            break;
    }

    if(!is_in_tight_list  ||  block->type != MD_BLOCK_P)
        MD_LEAVE_BLOCK(block->type, (void*) &det);

abort:
    if(clean_fence_code_detail) {
        md_free_attribute(ctx, &info_build);
        md_free_attribute(ctx, &lang_build);
    }
    return ret;
}

static int
md_process_all_blocks(MD_CTX* ctx)
{
    int byte_off = 0;
    int ret = 0;

    /* ctx->containers now is not needed for detection of lists and list items
     * so we reuse it for tracking what lists are loose or tight. We rely
     * on the fact the vector is large enough to hold the deepest nesting
     * level of lists. */
    ctx->n_containers = 0;

    while(byte_off < ctx->n_block_bytes) {
        MD_BLOCK* block = (MD_BLOCK*)((char*)ctx->block_bytes + byte_off);
        union {
            MD_BLOCK_UL_DETAIL ul;
            MD_BLOCK_OL_DETAIL ol;
            MD_BLOCK_LI_DETAIL li;
        } det;

        switch(block->type) {
            case MD_BLOCK_UL:
                det.ul.is_tight = (block->flags & MD_BLOCK_LOOSE_LIST) ? FALSE : TRUE;
                det.ul.mark = (CHAR) block->data;
                break;

            case MD_BLOCK_OL:
                det.ol.start = block->n_lines;
                det.ol.is_tight =  (block->flags & MD_BLOCK_LOOSE_LIST) ? FALSE : TRUE;
                det.ol.mark_delimiter = (CHAR) block->data;
                break;

            case MD_BLOCK_LI:
                det.li.is_task = (block->data != 0);
                det.li.task_mark = (CHAR) block->data;
                det.li.task_mark_offset = (OFF) block->n_lines;
                break;

            default:
                /* noop */
                break;
        }

        if(block->flags & MD_BLOCK_CONTAINER) {
            if(block->flags & MD_BLOCK_CONTAINER_CLOSER) {
                MD_LEAVE_BLOCK(block->type, &det);

                if(block->type == MD_BLOCK_UL || block->type == MD_BLOCK_OL || block->type == MD_BLOCK_QUOTE)
                    ctx->n_containers--;
            }

            if(block->flags & MD_BLOCK_CONTAINER_OPENER) {
                MD_ENTER_BLOCK(block->type, &det);

                if(block->type == MD_BLOCK_UL || block->type == MD_BLOCK_OL) {
                    ctx->containers[ctx->n_containers].is_loose = (block->flags & MD_BLOCK_LOOSE_LIST);
                    ctx->n_containers++;
                } else if(block->type == MD_BLOCK_QUOTE) {
                    /* This causes that any text in a block quote, even if
                     * nested inside a tight list item, is wrapped with
                     * <p>...</p>. */
                    ctx->containers[ctx->n_containers].is_loose = TRUE;
                    ctx->n_containers++;
                }
            }
        } else {
            MD_CHECK(md_process_leaf_block(ctx, block));

            if(block->type == MD_BLOCK_CODE || block->type == MD_BLOCK_HTML)
                byte_off += block->n_lines * sizeof(MD_VERBATIMLINE);
            else
                byte_off += block->n_lines * sizeof(MD_LINE);
        }

        byte_off += sizeof(MD_BLOCK);
    }

    ctx->n_block_bytes = 0;

abort:
    return ret;
}


/************************************
 ***  Grouping Lines into Blocks  ***
 ************************************/

static void*
md_push_block_bytes(MD_CTX* ctx, int n_bytes)
{
    void* ptr;

    if(ctx->n_block_bytes + n_bytes > ctx->alloc_block_bytes) {
        void* new_block_bytes;

        ctx->alloc_block_bytes = (ctx->alloc_block_bytes > 0 ? ctx->alloc_block_bytes * 2 : 512);
        new_block_bytes = realloc(ctx->block_bytes, ctx->alloc_block_bytes);
        if(new_block_bytes == NULL) {
            MD_LOG("realloc() failed.");
            return NULL;
        }

        /* Fix the ->current_block after the reallocation. */
        if(ctx->current_block != NULL) {
            OFF off_current_block = (char*) ctx->current_block - (char*) ctx->block_bytes;
            ctx->current_block = (MD_BLOCK*) ((char*) new_block_bytes + off_current_block);
        }

        ctx->block_bytes = new_block_bytes;
    }

    ptr = (char*)ctx->block_bytes + ctx->n_block_bytes;
    ctx->n_block_bytes += n_bytes;
    return ptr;
}

static int
md_start_new_block(MD_CTX* ctx, const MD_LINE_ANALYSIS* line)
{
    MD_BLOCK* block;

    MD_ASSERT(ctx->current_block == NULL);

    block = (MD_BLOCK*) md_push_block_bytes(ctx, sizeof(MD_BLOCK));
    if(block == NULL)
        return -1;

    switch(line->type) {
        case MD_LINE_HR:
            block->type = MD_BLOCK_HR;
            break;

        case MD_LINE_ATXHEADER:
        case MD_LINE_SETEXTHEADER:
            block->type = MD_BLOCK_H;
            break;

        case MD_LINE_FENCEDCODE:
        case MD_LINE_INDENTEDCODE:
            block->type = MD_BLOCK_CODE;
            break;

        case MD_LINE_TEXT:
            block->type = MD_BLOCK_P;
            break;

        case MD_LINE_HTML:
            block->type = MD_BLOCK_HTML;
            break;

        case MD_LINE_BLANK:
        case MD_LINE_SETEXTUNDERLINE:
        case MD_LINE_TABLEUNDERLINE:
        default:
            MD_UNREACHABLE();
            break;
    }

    block->flags = 0;
    block->data = line->data;
    block->n_lines = 0;

    ctx->current_block = block;
    return 0;
}

/* Eat from start of current (textual) block any reference definitions and
 * remember them so we can resolve any links referring to them.
 *
 * (Reference definitions can only be at start of it as they cannot break
 * a paragraph.)
 */
static int
md_consume_link_reference_definitions(MD_CTX* ctx)
{
    MD_LINE* lines = (MD_LINE*) (ctx->current_block + 1);
    int n_lines = ctx->current_block->n_lines;
    int n = 0;

    /* Compute how many lines at the start of the block form one or more
     * reference definitions. */
    while(n < n_lines) {
        int n_link_ref_lines;

        n_link_ref_lines = md_is_link_reference_definition(ctx,
                                    lines + n, n_lines - n);
        /* Not a reference definition? */
        if(n_link_ref_lines == 0)
            break;

        /* We fail if it is the ref. def. but it could not be stored due
         * a memory allocation error. */
        if(n_link_ref_lines < 0)
            return -1;

        n += n_link_ref_lines;
    }

    /* If there was at least one reference definition, we need to remove
     * its lines from the block, or perhaps even the whole block. */
    if(n > 0) {
        if(n == n_lines) {
            /* Remove complete block. */
            ctx->n_block_bytes -= n * sizeof(MD_LINE);
            ctx->n_block_bytes -= sizeof(MD_BLOCK);
        } else {
            /* Remove just some initial lines from the block. */
            memmove(lines, lines + n, (n_lines - n) * sizeof(MD_LINE));
            ctx->current_block->n_lines -= n;
            ctx->n_block_bytes -= n * sizeof(MD_LINE);
        }
    }

    return 0;
}

static int
md_end_current_block(MD_CTX* ctx)
{
    int ret = 0;

    if(ctx->current_block == NULL)
        return ret;

    /* Check whether there is a reference definition. (We do this here instead
     * of in md_analyze_line() because reference definition can take multiple
     * lines.) */
    if(ctx->current_block->type == MD_BLOCK_P) {
        MD_LINE* lines = (MD_LINE*) (ctx->current_block + 1);
        if(CH(lines[0].beg) == _T('['))
            MD_CHECK(md_consume_link_reference_definitions(ctx));
    }

    /* Mark we are not building any block anymore. */
    ctx->current_block = NULL;

abort:
    return ret;
}

static int
md_add_line_into_current_block(MD_CTX* ctx, const MD_LINE_ANALYSIS* analysis)
{
    MD_ASSERT(ctx->current_block != NULL);

    if(ctx->current_block->type == MD_BLOCK_CODE || ctx->current_block->type == MD_BLOCK_HTML) {
        MD_VERBATIMLINE* line;

        line = (MD_VERBATIMLINE*) md_push_block_bytes(ctx, sizeof(MD_VERBATIMLINE));
        if(line == NULL)
            return -1;

        line->indent = analysis->indent;
        line->beg = analysis->beg;
        line->end = analysis->end;
    } else {
        MD_LINE* line;

        line = (MD_LINE*) md_push_block_bytes(ctx, sizeof(MD_LINE));
        if(line == NULL)
            return -1;

        line->beg = analysis->beg;
        line->end = analysis->end;
    }
    ctx->current_block->n_lines++;

    return 0;
}

static int
md_push_container_bytes(MD_CTX* ctx, MD_BLOCKTYPE type, unsigned start,
                        unsigned data, unsigned flags)
{
    MD_BLOCK* block;
    int ret = 0;

    MD_CHECK(md_end_current_block(ctx));

    block = (MD_BLOCK*) md_push_block_bytes(ctx, sizeof(MD_BLOCK));
    if(block == NULL)
        return -1;

    block->type = type;
    block->flags = flags;
    block->data = data;
    block->n_lines = start;

abort:
    return ret;
}



/***********************
 ***  Line Analysis  ***
 ***********************/

static int
md_is_hr_line(MD_CTX* ctx, OFF beg, OFF* p_end)
{
    OFF off = beg + 1;
    int n = 1;

    while(off < ctx->size  &&  (CH(off) == CH(beg) || CH(off) == _T(' ') || CH(off) == _T('\t'))) {
        if(CH(off) == CH(beg))
            n++;
        off++;
    }

    if(n < 3)
        return FALSE;

    /* Nothing else can be present on the line. */
    if(off < ctx->size  &&  !ISNEWLINE(off))
        return FALSE;

    *p_end = off;
    return TRUE;
}

static int
md_is_atxheader_line(MD_CTX* ctx, OFF beg, OFF* p_beg, OFF* p_end, unsigned* p_level)
{
    int n;
    OFF off = beg + 1;

    while(off < ctx->size  &&  CH(off) == _T('#')  &&  off - beg < 7)
        off++;
    n = off - beg;

    if(n > 6)
        return FALSE;
    *p_level = n;

    if(!(ctx->parser.flags & MD_FLAG_PERMISSIVEATXHEADERS)  &&  off < ctx->size  &&
       CH(off) != _T(' ')  &&  CH(off) != _T('\t')  &&  !ISNEWLINE(off))
        return FALSE;

    while(off < ctx->size  &&  CH(off) == _T(' '))
        off++;
    *p_beg = off;
    *p_end = off;
    return TRUE;
}

static int
md_is_setext_underline(MD_CTX* ctx, OFF beg, OFF* p_end, unsigned* p_level)
{
    OFF off = beg + 1;

    while(off < ctx->size  &&  CH(off) == CH(beg))
        off++;

    while(off < ctx->size  && CH(off) == _T(' '))
        off++;

    /* Optionally, space(s) can follow. */
    while(off < ctx->size  &&  CH(off) == _T(' '))
        off++;

    /* But nothing more is allowed on the line. */
    if(off < ctx->size  &&  !ISNEWLINE(off))
        return FALSE;

    *p_level = (CH(beg) == _T('=') ? 1 : 2);
    *p_end = off;
    return TRUE;
}

static int
md_is_table_underline(MD_CTX* ctx, OFF beg, OFF* p_end, unsigned* p_col_count)
{
    OFF off = beg;
    int found_pipe = FALSE;
    unsigned col_count = 0;

    if(off < ctx->size  &&  CH(off) == _T('|')) {
        found_pipe = TRUE;
        off++;
        while(off < ctx->size  &&  ISWHITESPACE(off))
            off++;
    }

    while(1) {
        OFF cell_beg;
        int delimited = FALSE;

        /* Cell underline ("-----", ":----", "----:" or ":----:") */
        cell_beg = off;
        if(off < ctx->size  &&  CH(off) == _T(':'))
            off++;
        while(off < ctx->size  &&  CH(off) == _T('-'))
            off++;
        if(off < ctx->size  &&  CH(off) == _T(':'))
            off++;
        if(off - cell_beg < 3)
            return FALSE;

        col_count++;

        /* Pipe delimiter (optional at the end of line). */
        while(off < ctx->size  &&  ISWHITESPACE(off))
            off++;
        if(off < ctx->size  &&  CH(off) == _T('|')) {
            delimited = TRUE;
            found_pipe =  TRUE;
            off++;
            while(off < ctx->size  &&  ISWHITESPACE(off))
                off++;
        }

        /* Success, if we reach end of line. */
        if(off >= ctx->size  ||  ISNEWLINE(off))
            break;

        if(!delimited)
            return FALSE;
    }

    if(!found_pipe)
        return FALSE;

    *p_end = off;
    *p_col_count = col_count;
    return TRUE;
}

static int
md_is_opening_code_fence(MD_CTX* ctx, OFF beg, OFF* p_end)
{
    OFF off = beg;

    while(off < ctx->size && CH(off) == CH(beg))
        off++;

    /* Fence must have at least three characters. */
    if(off - beg < 3)
        return FALSE;

    ctx->code_fence_length = off - beg;

    /* Optionally, space(s) can follow. */
    while(off < ctx->size  &&  CH(off) == _T(' '))
        off++;

    /* Optionally, an info string can follow. It must not contain '`'. */
    while(off < ctx->size  &&  CH(off) != _T('`')  &&  !ISNEWLINE(off))
        off++;
    if(off < ctx->size  &&  !ISNEWLINE(off))
        return FALSE;

    *p_end = off;
    return TRUE;
}

static int
md_is_closing_code_fence(MD_CTX* ctx, CHAR ch, OFF beg, OFF* p_end)
{
    OFF off = beg;
    int ret = FALSE;

    /* Closing fence must have at least the same length and use same char as
     * opening one. */
    while(off < ctx->size  &&  CH(off) == ch)
        off++;
    if(off - beg < ctx->code_fence_length)
        goto out;

    /* Optionally, space(s) can follow */
    while(off < ctx->size  &&  CH(off) == _T(' '))
        off++;

    /* But nothing more is allowed on the line. */
    if(off < ctx->size  &&  !ISNEWLINE(off))
        goto out;

    ret = TRUE;

out:
    /* Note we set *p_end even on failure: If we are not closing fence, caller
     * would eat the line anyway without any parsing. */
    *p_end = off;
    return ret;
}

/* Returns type of the raw HTML block, or FALSE if it is not HTML block.
 * (Refer to CommonMark specification for details about the types.)
 */
static int
md_is_html_block_start_condition(MD_CTX* ctx, OFF beg)
{
    typedef struct TAG_tag TAG;
    struct TAG_tag {
        const CHAR* name;
        unsigned len    : 8;
    };

    /* Type 6 is started by a long list of allowed tags. We use two-level
     * tree to speed-up the search. */
#ifdef X
    #undef X
#endif
#define X(name)     { _T(name), sizeof(name)-1 }
#define Xend        { NULL, 0 }
    static const TAG t1[] = { X("script"), X("pre"), X("style"), Xend };

    static const TAG a6[] = { X("address"), X("article"), X("aside"), Xend };
    static const TAG b6[] = { X("base"), X("basefont"), X("blockquote"), X("body"), Xend };
    static const TAG c6[] = { X("caption"), X("center"), X("col"), X("colgroup"), Xend };
    static const TAG d6[] = { X("dd"), X("details"), X("dialog"), X("dir"),
                              X("div"), X("dl"), X("dt"), Xend };
    static const TAG f6[] = { X("fieldset"), X("figcaption"), X("figure"), X("footer"),
                              X("form"), X("frame"), X("frameset"), Xend };
    static const TAG h6[] = { X("h1"), X("head"), X("header"), X("hr"), X("html"), Xend };
    static const TAG i6[] = { X("iframe"), Xend };
    static const TAG l6[] = { X("legend"), X("li"), X("link"), Xend };
    static const TAG m6[] = { X("main"), X("menu"), X("menuitem"), X("meta"), Xend };
    static const TAG n6[] = { X("nav"), X("noframes"), Xend };
    static const TAG o6[] = { X("ol"), X("optgroup"), X("option"), Xend };
    static const TAG p6[] = { X("p"), X("param"), Xend };
    static const TAG s6[] = { X("section"), X("source"), X("summary"), Xend };
    static const TAG t6[] = { X("table"), X("tbody"), X("td"), X("tfoot"), X("th"),
                              X("thead"), X("title"), X("tr"), X("track"), Xend };
    static const TAG u6[] = { X("ul"), Xend };
    static const TAG xx[] = { Xend };
#undef X

    static const TAG* map6[26] = {
        a6, b6, c6, d6, xx, f6, xx, h6, i6, xx, xx, l6, m6,
        n6, o6, p6, xx, xx, s6, t6, u6, xx, xx, xx, xx, xx
    };
    OFF off = beg + 1;
    int i;

    /* Check for type 1: <script, <pre, or <style */
    for(i = 0; t1[i].name != NULL; i++) {
        if(off + t1[i].len < ctx->size) {
            if(md_ascii_case_eq(STR(off), t1[i].name, t1[i].len))
                return 1;
        }
    }

    /* Check for type 2: <!-- */
    if(off + 3 < ctx->size  &&  CH(off) == _T('!')  &&  CH(off+1) == _T('-')  &&  CH(off+2) == _T('-'))
        return 2;

    /* Check for type 3: <? */
    if(off < ctx->size  &&  CH(off) == _T('?'))
        return 3;

    /* Check for type 4 or 5: <! */
    if(off < ctx->size  &&  CH(off) == _T('!')) {
        /* Check for type 4: <! followed by uppercase letter. */
        if(off + 1 < ctx->size  &&  ISUPPER(off+1))
            return 4;

        /* Check for type 5: <![CDATA[ */
        if(off + 8 < ctx->size) {
            if(md_ascii_eq(STR(off), _T("![CDATA["), 8 * sizeof(CHAR)))
                return 5;
        }
    }

    /* Check for type 6: Many possible starting tags listed above. */
    if(off + 1 < ctx->size  &&  (ISALPHA(off) || (CH(off) == _T('/') && ISALPHA(off+1)))) {
        int slot;
        const TAG* tags;

        if(CH(off) == _T('/'))
            off++;

        slot = (ISUPPER(off) ? CH(off) - 'A' : CH(off) - 'a');
        tags = map6[slot];

        for(i = 0; tags[i].name != NULL; i++) {
            if(off + tags[i].len <= ctx->size) {
                if(md_ascii_case_eq(STR(off), tags[i].name, tags[i].len)) {
                    OFF tmp = off + tags[i].len;
                    if(tmp >= ctx->size)
                        return 6;
                    if(ISBLANK(tmp) || ISNEWLINE(tmp) || CH(tmp) == _T('>'))
                        return 6;
                    if(tmp+1 < ctx->size && CH(tmp) == _T('/') && CH(tmp+1) == _T('>'))
                        return 6;
                    break;
                }
            }
        }
    }

    /* Check for type 7: any COMPLETE other opening or closing tag. */
    if(off + 1 < ctx->size) {
        OFF end;

        if(md_is_html_tag(ctx, NULL, 0, beg, ctx->size, &end)) {
            /* Only optional whitespace and new line may follow. */
            while(end < ctx->size  &&  ISWHITESPACE(end))
                end++;
            if(end >= ctx->size  ||  ISNEWLINE(end))
                return 7;
        }
    }

    return FALSE;
}

/* Case sensitive check whether there is a substring 'what' between 'beg'
 * and end of line. */
static int
md_line_contains(MD_CTX* ctx, OFF beg, const CHAR* what, SZ what_len, OFF* p_end)
{
    OFF i;
    for(i = beg; i + what_len < ctx->size; i++) {
        if(ISNEWLINE(i))
            break;
        if(memcmp(STR(i), what, what_len * sizeof(CHAR)) == 0) {
            *p_end = i + what_len;
            return TRUE;
        }
    }

    *p_end = i;
    return FALSE;
}

/* Returns type of HTML block end condition or FALSE if not an end condition.
 *
 * Note it fills p_end even when it is not end condition as the caller
 * does not need to analyze contents of a raw HTML block.
 */
static int
md_is_html_block_end_condition(MD_CTX* ctx, OFF beg, OFF* p_end)
{
    switch(ctx->html_block_type) {
        case 1:
        {
            OFF off = beg;

            while(off < ctx->size  &&  !ISNEWLINE(off)) {
                if(CH(off) == _T('<')) {
                    if(md_ascii_case_eq(STR(off), _T("</script>"), 9)) {
                        *p_end = off + 9;
                        return TRUE;
                    }

                    if(md_ascii_case_eq(STR(off), _T("</style>"), 8)) {
                        *p_end = off + 8;
                        return TRUE;
                    }

                    if(md_ascii_case_eq(STR(off), _T("</pre>"), 6)) {
                        *p_end = off + 6;
                        return TRUE;
                    }
                }

                off++;
            }
            *p_end = off;
            return FALSE;
        }

        case 2:
            return (md_line_contains(ctx, beg, _T("-->"), 3, p_end) ? 2 : FALSE);

        case 3:
            return (md_line_contains(ctx, beg, _T("?>"), 2, p_end) ? 3 : FALSE);

        case 4:
            return (md_line_contains(ctx, beg, _T(">"), 1, p_end) ? 4 : FALSE);

        case 5:
            return (md_line_contains(ctx, beg, _T("]]>"), 3, p_end) ? 5 : FALSE);

        case 6:     /* Pass through */
        case 7:
            *p_end = beg;
            return (ISNEWLINE(beg) ? ctx->html_block_type : FALSE);

        default:
            MD_UNREACHABLE();
    }
    return FALSE;
}


static int
md_is_container_compatible(const MD_CONTAINER* pivot, const MD_CONTAINER* container)
{
    /* Block quote has no "items" like lists. */
    if(container->ch == _T('>'))
        return FALSE;

    if(container->ch != pivot->ch)
        return FALSE;
    if(container->mark_indent > pivot->contents_indent)
        return FALSE;

    return TRUE;
}

static int
md_push_container(MD_CTX* ctx, const MD_CONTAINER* container)
{
    if(ctx->n_containers >= ctx->alloc_containers) {
        MD_CONTAINER* new_containers;

        ctx->alloc_containers = (ctx->alloc_containers > 0 ? ctx->alloc_containers * 2 : 16);
        new_containers = realloc(ctx->containers, ctx->alloc_containers * sizeof(MD_CONTAINER));
        if(new_containers == NULL) {
            MD_LOG("realloc() failed.");
            return -1;
        }

        ctx->containers = new_containers;
    }

    memcpy(&ctx->containers[ctx->n_containers++], container, sizeof(MD_CONTAINER));
    return 0;
}

static int
md_enter_child_containers(MD_CTX* ctx, int n_children, unsigned data)
{
    int i;
    int ret = 0;

    for(i = ctx->n_containers - n_children; i < ctx->n_containers; i++) {
        MD_CONTAINER* c = &ctx->containers[i];
        int is_ordered_list = FALSE;

        switch(c->ch) {
            case _T(')'):
            case _T('.'):
                is_ordered_list = TRUE;
                /* Pass through */

            case _T('-'):
            case _T('+'):
            case _T('*'):
                /* Remember offset in ctx->block_bytes so we can revisit the
                 * block if we detect it is a loose list. */
                md_end_current_block(ctx);
                c->block_byte_off = ctx->n_block_bytes;

                MD_CHECK(md_push_container_bytes(ctx,
                                (is_ordered_list ? MD_BLOCK_OL : MD_BLOCK_UL),
                                c->start, data, MD_BLOCK_CONTAINER_OPENER));
                MD_CHECK(md_push_container_bytes(ctx, MD_BLOCK_LI,
                                c->task_mark_off,
                                (c->is_task ? CH(c->task_mark_off) : 0),
                                MD_BLOCK_CONTAINER_OPENER));
                break;

            case _T('>'):
                MD_CHECK(md_push_container_bytes(ctx, MD_BLOCK_QUOTE, 0, 0, MD_BLOCK_CONTAINER_OPENER));
                break;

            default:
                MD_UNREACHABLE();
                break;
        }
    }

abort:
    return ret;
}

static int
md_leave_child_containers(MD_CTX* ctx, int n_keep)
{
    int ret = 0;

    while(ctx->n_containers > n_keep) {
        MD_CONTAINER* c = &ctx->containers[ctx->n_containers-1];
        int is_ordered_list = FALSE;

        switch(c->ch) {
            case _T(')'):
            case _T('.'):
                is_ordered_list = TRUE;
                /* Pass through */

            case _T('-'):
            case _T('+'):
            case _T('*'):
                MD_CHECK(md_push_container_bytes(ctx, MD_BLOCK_LI,
                                c->task_mark_off, (c->is_task ? CH(c->task_mark_off) : 0),
                                MD_BLOCK_CONTAINER_CLOSER));
                MD_CHECK(md_push_container_bytes(ctx,
                                (is_ordered_list ? MD_BLOCK_OL : MD_BLOCK_UL), 0,
                                c->ch, MD_BLOCK_CONTAINER_CLOSER));
                break;

            case _T('>'):
                MD_CHECK(md_push_container_bytes(ctx, MD_BLOCK_QUOTE, 0,
                                0, MD_BLOCK_CONTAINER_CLOSER));
                break;

            default:
                MD_UNREACHABLE();
                break;
        }

        ctx->n_containers--;
    }

abort:
    return ret;
}

static int
md_is_container_mark(MD_CTX* ctx, unsigned indent, OFF beg, OFF* p_end, MD_CONTAINER* p_container)
{
    OFF off = beg;
    OFF max_end;

    /* Check for block quote mark. */
    if(off < ctx->size  &&  CH(off) == _T('>')) {
        off++;
        p_container->ch = _T('>');
        p_container->is_loose = FALSE;
        p_container->is_task = FALSE;
        p_container->mark_indent = indent;
        p_container->contents_indent = indent + 1;
        *p_end = off;
        return TRUE;
    }

    /* Check for list item bullet mark. */
    if(off+1 < ctx->size  &&  ISANYOF(off, _T("-+*"))  &&  (ISBLANK(off+1) || ISNEWLINE(off+1))) {
        p_container->ch = CH(off);
        p_container->is_loose = FALSE;
        p_container->is_task = FALSE;
        p_container->mark_indent = indent;
        p_container->contents_indent = indent + 1;
        *p_end = off + 1;
        return TRUE;
    }

    /* Check for ordered list item marks. */
    max_end = off + 9;
    if(max_end > ctx->size)
        max_end = ctx->size;
    p_container->start = 0;
    while(off < max_end  &&  ISDIGIT(off)) {
        p_container->start = p_container->start * 10 + CH(off) - _T('0');
        off++;
    }
    if(off+1 < ctx->size  &&  (CH(off) == _T('.') || CH(off) == _T(')'))   &&  (ISBLANK(off+1) || ISNEWLINE(off+1))) {
        p_container->ch = CH(off);
        p_container->is_loose = FALSE;
        p_container->is_task = FALSE;
        p_container->mark_indent = indent;
        p_container->contents_indent = indent + off - beg + 1;
        *p_end = off + 1;
        return TRUE;
    }

    return FALSE;
}

static unsigned
md_line_indentation(MD_CTX* ctx, unsigned total_indent, OFF beg, OFF* p_end)
{
    OFF off = beg;
    unsigned indent = total_indent;

    while(off < ctx->size  &&  ISBLANK(off)) {
        if(CH(off) == _T('\t'))
            indent = (indent + 4) & ~3;
        else
            indent++;
        off++;
    }

    *p_end = off;
    return indent - total_indent;
}

static const MD_LINE_ANALYSIS md_dummy_blank_line = { MD_LINE_BLANK, 0 };

/* Analyze type of the line and find some its properties. This serves as a
 * main input for determining type and boundaries of a block. */
static int
md_analyze_line(MD_CTX* ctx, OFF beg, OFF* p_end,
                const MD_LINE_ANALYSIS* pivot_line, MD_LINE_ANALYSIS* line)
{
    unsigned total_indent = 0;
    int n_parents = 0;
    int n_brothers = 0;
    int n_children = 0;
    MD_CONTAINER container = { 0 };
    int prev_line_has_list_loosening_effect = ctx->last_line_has_list_loosening_effect;
    OFF off = beg;
    int ret = 0;

    line->indent = md_line_indentation(ctx, total_indent, off, &off);
    total_indent += line->indent;
    line->beg = off;

    /* Given the indentation and block quote marks '>', determine how many of
     * the current containers are our parents. */
    while(n_parents < ctx->n_containers) {
        MD_CONTAINER* c = &ctx->containers[n_parents];

        if(c->ch == _T('>')  &&  line->indent < ctx->code_indent_offset  &&
            off < ctx->size  &&  CH(off) == _T('>'))
        {
            /* Block quote mark. */
            off++;
            total_indent++;
            line->indent = md_line_indentation(ctx, total_indent, off, &off);
            total_indent += line->indent;

            /* The optional 1st space after '>' is part of the block quote mark. */
            if(line->indent > 0)
                line->indent--;

            line->beg = off;
        } else if(c->ch != _T('>')  &&  line->indent >= c->contents_indent) {
            /* List. */
            line->indent -= c->contents_indent;
        } else {
            break;
        }

        n_parents++;
    }

redo:
    /* Check whether we are fenced code continuation. */
    if(pivot_line->type == MD_LINE_FENCEDCODE) {
        line->beg = off;

        /* We are another MD_LINE_FENCEDCODE unless we are closing fence
         * which we transform into MD_LINE_BLANK. */
        if(line->indent < ctx->code_indent_offset) {
            if(md_is_closing_code_fence(ctx, CH(pivot_line->beg), off, &off)) {
                line->type = MD_LINE_BLANK;
                ctx->last_line_has_list_loosening_effect = FALSE;
                goto done;
            }
        }

        if(off >= ctx->size  ||  ISNEWLINE(off)) {
            /* Blank line does not need any real indentation to be nested inside
             * a list. */
            if(n_brothers + n_children == 0) {
                while(n_parents < ctx->n_containers  &&  ctx->containers[n_parents].ch != _T('>'))
                    n_parents++;
            }
        }

        /* Change indentation accordingly to the initial code fence. */
        if(n_parents == ctx->n_containers) {
            if(line->indent > pivot_line->indent)
                line->indent -= pivot_line->indent;
            else
                line->indent = 0;

            line->type = MD_LINE_FENCEDCODE;
            goto done;
        }
    }

    /* Check whether we are HTML block continuation. */
    if(pivot_line->type == MD_LINE_HTML  &&  ctx->html_block_type > 0) {
        int html_block_type;

        html_block_type = md_is_html_block_end_condition(ctx, off, &off);
        if(html_block_type > 0) {
            MD_ASSERT(html_block_type == ctx->html_block_type);

            /* Make sure this is the last line of the block. */
            ctx->html_block_type = 0;

            /* Some end conditions serve as blank lines at the same time. */
            if(html_block_type == 6 || html_block_type == 7) {
                line->type = MD_LINE_BLANK;
                line->indent = 0;
                goto done;
            }
        }

        if(n_parents == ctx->n_containers) {
            line->type = MD_LINE_HTML;
            goto done;
        }
    }

    /* Check for blank line. */
    if(off >= ctx->size  ||  ISNEWLINE(off)) {
        /* Blank line does not need any real indentation to be nested inside
         * a list. */
        if(n_brothers + n_children == 0) {
            while(n_parents < ctx->n_containers  &&  ctx->containers[n_parents].ch != _T('>'))
                n_parents++;
        }

        if(pivot_line->type == MD_LINE_INDENTEDCODE  &&  n_parents == ctx->n_containers) {
            line->type = MD_LINE_INDENTEDCODE;
            if(line->indent > ctx->code_indent_offset)
                line->indent -= ctx->code_indent_offset;
            else
                line->indent = 0;
            ctx->last_line_has_list_loosening_effect = FALSE;
        } else {
            line->type = MD_LINE_BLANK;
            ctx->last_line_has_list_loosening_effect = (n_parents > 0  &&
                    n_brothers + n_children == 0  &&
                    ctx->containers[n_parents-1].ch != _T('>'));

#if 1
            /* See https://github.com/mity/md4c/issues/6
             *
             * This ugly checking tests we are in (yet empty) list item but not
             * its very first line (with the list item mark).
             *
             * If we are such blank line, then any following non-blank line
             * which would be part of this list item actually ends the list
             * because "a list item can begin with at most one blank line."
             */
            if(n_parents > 0  &&  ctx->containers[n_parents-1].ch != _T('>')  &&
               n_brothers + n_children == 0  &&  ctx->current_block == NULL  &&
               ctx->n_block_bytes > sizeof(MD_BLOCK))
            {
                MD_BLOCK* top_block = (MD_BLOCK*) ((char*)ctx->block_bytes + ctx->n_block_bytes - sizeof(MD_BLOCK));
                if(top_block->type == MD_BLOCK_LI)
                    ctx->last_list_item_starts_with_two_blank_lines = TRUE;
            }
#endif
        }
        goto done_on_eol;
    } else {
#if 1
        /* This is 2nd half of the hack. If the flag is set (that is there
         * were 2nd blank line at the start of the list item) and we would also
         * belonging to such list item, then interrupt the list. */
        ctx->last_line_has_list_loosening_effect = FALSE;
        if(ctx->last_list_item_starts_with_two_blank_lines) {
            if(n_parents > 0  &&  ctx->containers[n_parents-1].ch != _T('>')  &&
               n_brothers + n_children == 0  &&  ctx->current_block == NULL  &&
               ctx->n_block_bytes > sizeof(MD_BLOCK))
            {
                MD_BLOCK* top_block = (MD_BLOCK*) ((char*)ctx->block_bytes + ctx->n_block_bytes - sizeof(MD_BLOCK));
                if(top_block->type == MD_BLOCK_LI)
                    n_parents--;
            }

            ctx->last_list_item_starts_with_two_blank_lines = FALSE;
        }
#endif
    }

    /* Check whether we are Setext underline. */
    if(line->indent < ctx->code_indent_offset  &&  pivot_line->type == MD_LINE_TEXT
        &&  (CH(off) == _T('=') || CH(off) == _T('-'))
        &&  (n_parents == ctx->n_containers))
    {
        unsigned level;

        if(md_is_setext_underline(ctx, off, &off, &level)) {
            line->type = MD_LINE_SETEXTUNDERLINE;
            line->data = level;
            goto done;
        }
    }

    /* Check for thematic break line. */
    if(line->indent < ctx->code_indent_offset  &&  ISANYOF(off, _T("-_*"))) {
        if(md_is_hr_line(ctx, off, &off)) {
            line->type = MD_LINE_HR;
            goto done;
        }
    }

    /* Check for "brother" container. I.e. whether we are another list item
     * in already started list. */
    if(n_parents < ctx->n_containers  &&  n_brothers + n_children == 0) {
        OFF tmp;

        if(md_is_container_mark(ctx, line->indent, off, &tmp, &container)  &&
           md_is_container_compatible(&ctx->containers[n_parents], &container))
        {
            pivot_line = &md_dummy_blank_line;

            off = tmp;

            total_indent += container.contents_indent - container.mark_indent;
            line->indent = md_line_indentation(ctx, total_indent, off, &off);
            total_indent += line->indent;
            line->beg = off;

            /* Some of the following whitespace actually still belongs to the mark. */
            if(off >= ctx->size || ISNEWLINE(off)) {
                container.contents_indent++;
            } else if(line->indent <= ctx->code_indent_offset) {
                container.contents_indent += line->indent;
                line->indent = 0;
            } else {
                container.contents_indent += 1;
                line->indent--;
            }

            ctx->containers[n_parents].mark_indent = container.mark_indent;
            ctx->containers[n_parents].contents_indent = container.contents_indent;

            n_brothers++;
            goto redo;
        }
    }

    /* Check for indented code.
     * Note indented code block cannot interrupt a paragraph. */
    if(line->indent >= ctx->code_indent_offset  &&
        (pivot_line->type == MD_LINE_BLANK || pivot_line->type == MD_LINE_INDENTEDCODE))
    {
        line->type = MD_LINE_INDENTEDCODE;
        MD_ASSERT(line->indent >= ctx->code_indent_offset);
        line->indent -= ctx->code_indent_offset;
        line->data = 0;
        goto done;
    }

    /* Check for start of a new container block. */
    if(line->indent < ctx->code_indent_offset  &&
       md_is_container_mark(ctx, line->indent, off, &off, &container))
    {
        if(pivot_line->type == MD_LINE_TEXT  &&  n_parents == ctx->n_containers  &&
                    (off >= ctx->size || ISNEWLINE(off)))
        {
            /* Noop. List mark followed by a blank line cannot interrupt a paragraph. */
        } else if(pivot_line->type == MD_LINE_TEXT  &&  n_parents == ctx->n_containers  &&
                    (container.ch == _T('.') || container.ch == _T(')'))  &&  container.start != 1)
        {
            /* Noop. Ordered list cannot interrupt a paragraph unless the start index is 1. */
        } else {
            total_indent += container.contents_indent - container.mark_indent;
            line->indent = md_line_indentation(ctx, total_indent, off, &off);
            total_indent += line->indent;

            line->beg = off;
            line->data = container.ch;

            /* Some of the following whitespace actually still belongs to the mark. */
            if(off >= ctx->size || ISNEWLINE(off)) {
                container.contents_indent++;
            } else if(line->indent <= ctx->code_indent_offset) {
                container.contents_indent += line->indent;
                line->indent = 0;
            } else {
                container.contents_indent += 1;
                line->indent--;
            }

            if(n_brothers + n_children == 0)
                pivot_line = &md_dummy_blank_line;

            if(n_children == 0)
                MD_CHECK(md_leave_child_containers(ctx, n_parents + n_brothers));

            n_children++;
            MD_CHECK(md_push_container(ctx, &container));
            goto redo;
        }
    }

    /* Check whether we are table continuation. */
    if(pivot_line->type == MD_LINE_TABLE  &&  md_is_table_row(ctx, off, &off)  &&
       n_parents == ctx->n_containers)
    {
        line->type = MD_LINE_TABLE;
        goto done;
    }

    /* Check for ATX header. */
    if(line->indent < ctx->code_indent_offset  &&  CH(off) == _T('#')) {
        unsigned level;

        if(md_is_atxheader_line(ctx, off, &line->beg, &off, &level)) {
            line->type = MD_LINE_ATXHEADER;
            line->data = level;
            goto done;
        }
    }

    /* Check whether we are starting code fence. */
    if(CH(off) == _T('`') || CH(off) == _T('~')) {
        if(md_is_opening_code_fence(ctx, off, &off)) {
            line->type = MD_LINE_FENCEDCODE;
            line->data = 1;
            goto done;
        }
    }

    /* Check for start of raw HTML block. */
    if(CH(off) == _T('<')  &&  !(ctx->parser.flags & MD_FLAG_NOHTMLBLOCKS))
    {
        ctx->html_block_type = md_is_html_block_start_condition(ctx, off);

        /* HTML block type 7 cannot interrupt paragraph. */
        if(ctx->html_block_type == 7  &&  pivot_line->type == MD_LINE_TEXT)
            ctx->html_block_type = 0;

        if(ctx->html_block_type > 0) {
            /* The line itself also may immediately close the block. */
            if(md_is_html_block_end_condition(ctx, off, &off) == ctx->html_block_type) {
                /* Make sure this is the last line of the block. */
                ctx->html_block_type = 0;
            }

            line->type = MD_LINE_HTML;
            goto done;
        }
    }

    /* Check for table underline. */
    if((ctx->parser.flags & MD_FLAG_TABLES)  &&  pivot_line->type == MD_LINE_TEXT  &&
       (CH(off) == _T('|') || CH(off) == _T('-') || CH(off) == _T(':'))  &&
       n_parents == ctx->n_containers)
    {
        unsigned col_count;

        if(ctx->current_block != NULL  &&  ctx->current_block->n_lines == 1  &&
            md_is_table_underline(ctx, off, &off, &col_count)  &&
            md_is_table_row(ctx, pivot_line->beg, NULL))
        {
            line->data = col_count;
            line->type = MD_LINE_TABLEUNDERLINE;
            goto done;
        }
    }

    /* By default, we are normal text line. */
    line->type = MD_LINE_TEXT;
    if(pivot_line->type == MD_LINE_TEXT  &&  n_brothers + n_children == 0) {
        /* Lazy continuation. */
        n_parents = ctx->n_containers;
    }

    /* Check for task mark. */
    if((ctx->parser.flags & MD_FLAG_TASKLISTS)  &&  n_brothers + n_children > 0  &&
       ISANYOF_(ctx->containers[ctx->n_containers-1].ch, _T("-+*.)")))
    {
        OFF tmp = off;

        while(tmp < ctx->size  &&  tmp < off + 3  &&  ISBLANK(tmp))
            tmp++;
        if(tmp + 2 < ctx->size  &&  CH(tmp) == _T('[')  &&
           ISANYOF(tmp+1, _T("xX "))  &&  CH(tmp+2) == _T(']')  &&
           (tmp + 3 == ctx->size  ||  ISBLANK(tmp+3)  ||  ISNEWLINE(tmp+3)))
        {
            MD_CONTAINER* task_container = (n_children > 0 ? &ctx->containers[ctx->n_containers-1] : &container);
            task_container->is_task = TRUE;
            task_container->task_mark_off = tmp + 1;
            off = tmp + 3;
            while(ISWHITESPACE(off))
                off++;
            line->beg = off;
        }
    }

done:
    /* Scan for end of the line.
     *
     * Note this is bottleneck of this function as we itereate over (almost)
     * all line contents after some initial line indentation. To optimize, we
     * try to eat multiple chars in every loop iteration.
     *
     * (Measured ~6% performance boost of md2html with this optimization for
     * normal kind of input.)
     */
    while(off + 4 < ctx->size  &&  !ISNEWLINE(off+0)  &&  !ISNEWLINE(off+1)
                               &&  !ISNEWLINE(off+2)  &&  !ISNEWLINE(off+3))
        off += 4;
    while(off < ctx->size  &&  !ISNEWLINE(off))
        off++;

done_on_eol:
    /* Set end of the line. */
    line->end = off;

    /* But for ATX header, we should exclude the optional trailing mark. */
    if(line->type == MD_LINE_ATXHEADER) {
        OFF tmp = line->end;
        while(tmp > line->beg && CH(tmp-1) == _T(' '))
            tmp--;
        while(tmp > line->beg && CH(tmp-1) == _T('#'))
            tmp--;
        if(tmp == line->beg || CH(tmp-1) == _T(' ') || (ctx->parser.flags & MD_FLAG_PERMISSIVEATXHEADERS))
            line->end = tmp;
    }

    /* Trim trailing spaces. */
    if(line->type != MD_LINE_INDENTEDCODE  &&  line->type != MD_LINE_FENCEDCODE) {
        while(line->end > line->beg && CH(line->end-1) == _T(' '))
            line->end--;
    }

    /* Eat also the new line. */
    if(off < ctx->size && CH(off) == _T('\r'))
        off++;
    if(off < ctx->size && CH(off) == _T('\n'))
        off++;

    *p_end = off;

    /* If we belong to a list after seeing a blank line, the list is loose. */
    if(prev_line_has_list_loosening_effect  &&  line->type != MD_LINE_BLANK  &&  n_parents + n_brothers > 0) {
        MD_CONTAINER* c = &ctx->containers[n_parents + n_brothers - 1];
        if(c->ch != _T('>')) {
            MD_BLOCK* block = (MD_BLOCK*) (((char*)ctx->block_bytes) + c->block_byte_off);
            block->flags |= MD_BLOCK_LOOSE_LIST;
        }
    }

    /* Leave any containers we are not part of anymore. */
    if(n_children == 0  &&  n_parents + n_brothers < ctx->n_containers)
        MD_CHECK(md_leave_child_containers(ctx, n_parents + n_brothers));

    /* Enter any container we found a mark for. */
    if(n_brothers > 0) {
        MD_ASSERT(n_brothers == 1);
        MD_CHECK(md_push_container_bytes(ctx, MD_BLOCK_LI,
                    ctx->containers[n_parents].task_mark_off,
                    (ctx->containers[n_parents].is_task ? CH(ctx->containers[n_parents].task_mark_off) : 0),
                    MD_BLOCK_CONTAINER_CLOSER));
        MD_CHECK(md_push_container_bytes(ctx, MD_BLOCK_LI,
                    container.task_mark_off,
                    (container.is_task ? CH(container.task_mark_off) : 0),
                    MD_BLOCK_CONTAINER_OPENER));
        ctx->containers[n_parents].is_task = container.is_task;
        ctx->containers[n_parents].task_mark_off = container.task_mark_off;
    }

    if(n_children > 0)
        MD_CHECK(md_enter_child_containers(ctx, n_children, line->data));

abort:
    return ret;
}

static int
md_process_line(MD_CTX* ctx, const MD_LINE_ANALYSIS** p_pivot_line, const MD_LINE_ANALYSIS* line)
{
    const MD_LINE_ANALYSIS* pivot_line = *p_pivot_line;
    int ret = 0;

    /* Blank line ends current leaf block. */
    if(line->type == MD_LINE_BLANK) {
        MD_CHECK(md_end_current_block(ctx));
        *p_pivot_line = &md_dummy_blank_line;
        return 0;
    }

    /* Some line types form block on their own. */
    if(line->type == MD_LINE_HR || line->type == MD_LINE_ATXHEADER) {
        MD_CHECK(md_end_current_block(ctx));

        /* Add our single-line block. */
        MD_CHECK(md_start_new_block(ctx, line));
        MD_CHECK(md_add_line_into_current_block(ctx, line));
        MD_CHECK(md_end_current_block(ctx));
        *p_pivot_line = &md_dummy_blank_line;
        return 0;
    }

    /* MD_LINE_SETEXTUNDERLINE changes meaning of the current block and ends it. */
    if(line->type == MD_LINE_SETEXTUNDERLINE) {
        MD_ASSERT(ctx->current_block != NULL);
        ctx->current_block->type = MD_BLOCK_H;
        ctx->current_block->data = line->data;
        MD_CHECK(md_end_current_block(ctx));
        *p_pivot_line = &md_dummy_blank_line;
        return 0;
    }

    /* MD_LINE_TABLEUNDERLINE changes meaning of the current block. */
    if(line->type == MD_LINE_TABLEUNDERLINE) {
        MD_ASSERT(ctx->current_block != NULL);
        MD_ASSERT(ctx->current_block->n_lines == 1);
        ctx->current_block->type = MD_BLOCK_TABLE;
        ctx->current_block->data = line->data;
        MD_ASSERT(pivot_line != &md_dummy_blank_line);
        ((MD_LINE_ANALYSIS*)pivot_line)->type = MD_LINE_TABLE;
        MD_CHECK(md_add_line_into_current_block(ctx, line));
        return 0;
    }

    /* The current block also ends if the line has different type. */
    if(line->type != pivot_line->type)
        MD_CHECK(md_end_current_block(ctx));

    /* The current line may start a new block. */
    if(ctx->current_block == NULL) {
        MD_CHECK(md_start_new_block(ctx, line));
        *p_pivot_line = line;
    }

    /* In all other cases the line is just a continuation of the current block. */
    MD_CHECK(md_add_line_into_current_block(ctx, line));

abort:
    return ret;
}

static int
md_process_doc(MD_CTX *ctx)
{
    const MD_LINE_ANALYSIS* pivot_line = &md_dummy_blank_line;
    MD_LINE_ANALYSIS line_buf[2];
    MD_LINE_ANALYSIS* line = &line_buf[0];
    OFF off = 0;
    int ret = 0;

    MD_ENTER_BLOCK(MD_BLOCK_DOC, NULL);

    while(off < ctx->size) {
        if(line == pivot_line)
            line = (line == &line_buf[0] ? &line_buf[1] : &line_buf[0]);

        MD_CHECK(md_analyze_line(ctx, off, &off, pivot_line, line));
        MD_CHECK(md_process_line(ctx, &pivot_line, line));
    }

    md_end_current_block(ctx);

    MD_CHECK(md_build_ref_def_hashtable(ctx));

    /* Process all blocks. */
    MD_CHECK(md_leave_child_containers(ctx, 0));
    MD_CHECK(md_process_all_blocks(ctx));

    MD_LEAVE_BLOCK(MD_BLOCK_DOC, NULL);

abort:

#if 0
    /* Output some memory consumption statistics. */
    {
        char buffer[256];
        sprintf(buffer, "Alloced %u bytes for block buffer.",
                    (unsigned)(ctx->alloc_block_bytes));
        MD_LOG(buffer);

        sprintf(buffer, "Alloced %u bytes for containers buffer.",
                    (unsigned)(ctx->alloc_containers * sizeof(MD_CONTAINER)));
        MD_LOG(buffer);

        sprintf(buffer, "Alloced %u bytes for marks buffer.",
                    (unsigned)(ctx->alloc_marks * sizeof(MD_MARK)));
        MD_LOG(buffer);

        sprintf(buffer, "Alloced %u bytes for aux. buffer.",
                    (unsigned)(ctx->alloc_buffer * sizeof(MD_CHAR)));
        MD_LOG(buffer);
    }
#endif

    return ret;
}


/********************
 ***  Public API  ***
 ********************/

int
md_parse(const MD_CHAR* text, MD_SIZE size, const MD_PARSER* parser, void* userdata)
{
    MD_CTX ctx;
    int i;
    int ret;

    if(parser->abi_version != 0) {
        if(parser->debug_log != NULL)
            parser->debug_log("Unsupported abi_version.", userdata);
        return -1;
    }

    /* Setup context structure. */
    memset(&ctx, 0, sizeof(MD_CTX));
    ctx.text = text;
    ctx.size = size;
    memcpy(&ctx.parser, parser, sizeof(MD_PARSER));
    ctx.userdata = userdata;
    ctx.code_indent_offset = (ctx.parser.flags & MD_FLAG_NOINDENTEDCODEBLOCKS) ? (OFF)(-1) : 4;
    md_build_mark_char_map(&ctx);

    /* Reset all unresolved opener mark chains. */
    for(i = 0; i < SIZEOF_ARRAY(ctx.mark_chains); i++) {
        ctx.mark_chains[i].head = -1;
        ctx.mark_chains[i].tail = -1;
    }
    ctx.unresolved_link_head = -1;
    ctx.unresolved_link_tail = -1;

    /* All the work. */
    ret = md_process_doc(&ctx);

    /* Clean-up. */
    md_free_ref_defs(&ctx);
    md_free_ref_def_hashtable(&ctx);
    free(ctx.buffer);
    free(ctx.marks);
    free(ctx.block_bytes);
    free(ctx.containers);

    return ret;
}
