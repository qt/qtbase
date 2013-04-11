/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "atom.h"

unsigned int
xkb_context_num_failed_include_paths(struct xkb_context *ctx);

const char *
xkb_context_failed_include_path_get(struct xkb_context *ctx,
                                    unsigned int idx);

/*
 * Returns XKB_ATOM_NONE if @string was not previously interned,
 * otherwise returns the atom.
 */
xkb_atom_t
xkb_atom_lookup(struct xkb_context *ctx, const char *string);

xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string);

/**
 * If @string is dynamically allocated, free'd immediately after
 * being interned, and not used afterwards, use this function
 * instead of xkb_atom_intern to avoid some unnecessary allocations.
 * The caller should not use or free the passed in string afterwards.
 */
xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string);

char *
xkb_atom_strdup(struct xkb_context *ctx, xkb_atom_t atom);

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom);

char *
xkb_context_get_buffer(struct xkb_context *ctx, size_t size);

ATTR_PRINTF(3, 4) void
xkb_log(struct xkb_context *ctx, enum xkb_log_level level,
        const char *fmt, ...);

#define xkb_log_cond_level(ctx, level, ...) do { \
    if (xkb_context_get_log_level(ctx) >= (level)) \
    xkb_log((ctx), (level), __VA_ARGS__); \
} while (0)

#define xkb_log_cond_verbosity(ctx, level, vrb, ...) do { \
    if (xkb_context_get_log_verbosity(ctx) >= (vrb)) \
    xkb_log_cond_level((ctx), (level), __VA_ARGS__); \
} while (0)

const char *
xkb_context_get_default_rules(struct xkb_context *ctx);

const char *
xkb_context_get_default_model(struct xkb_context *ctx);

const char *
xkb_context_get_default_layout(struct xkb_context *ctx);

const char *
xkb_context_get_default_variant(struct xkb_context *ctx);

const char *
xkb_context_get_default_options(struct xkb_context *ctx);

/*
 * The format is not part of the argument list in order to avoid the
 * "ISO C99 requires rest arguments to be used" warning when only the
 * format is supplied without arguments. Not supplying it would still
 * result in an error, though.
 */
#define log_dbg(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_INFO, __VA_ARGS__)
#define log_warn(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_WARNING, __VA_ARGS__)
#define log_err(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_ERROR, __VA_ARGS__)
#define log_wsgo(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define log_vrb(ctx, vrb, ...) \
    xkb_log_cond_verbosity((ctx), XKB_LOG_LEVEL_WARNING, (vrb), __VA_ARGS__)

/*
 * Variants which are prefixed by the name of the function they're
 * called from.
 * Here we must have the silly 1 variant.
 */
#define log_err_func(ctx, fmt, ...) \
    log_err(ctx, "%s: " fmt, __func__, __VA_ARGS__)
#define log_err_func1(ctx, fmt) \
    log_err(ctx, "%s: " fmt, __func__)

#endif
