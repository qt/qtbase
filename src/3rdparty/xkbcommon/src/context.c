/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita
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

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "context.h"

struct xkb_context {
    int refcnt;

    ATTR_PRINTF(3, 0) void (*log_fn)(struct xkb_context *ctx,
                                     enum xkb_log_level level,
                                     const char *fmt, va_list args);
    enum xkb_log_level log_level;
    int log_verbosity;
    void *user_data;

    struct xkb_rule_names names_dflt;

    darray(char *) includes;
    darray(char *) failed_includes;

    struct atom_table *atom_table;

    /* Buffer for the *Text() functions. */
    char text_buffer[2048];
    size_t text_next;

    unsigned int use_environment_names : 1;
};

/**
 * Append one directory to the context's include path.
 */
XKB_EXPORT int
xkb_context_include_path_append(struct xkb_context *ctx, const char *path)
{
    struct stat stat_buf;
    int err;
    char *tmp;

    tmp = strdup(path);
    if (!tmp)
        goto err;

    err = stat(path, &stat_buf);
    if (err != 0)
        goto err;
    if (!S_ISDIR(stat_buf.st_mode))
        goto err;

#if defined(HAVE_EACCESS)
    if (eaccess(path, R_OK | X_OK) != 0)
        goto err;
#elif defined(HAVE_EUIDACCESS)
    if (euidaccess(path, R_OK | X_OK) != 0)
        goto err;
#endif

    darray_append(ctx->includes, tmp);
    return 1;

err:
    darray_append(ctx->failed_includes, tmp);
    return 0;
}

/**
 * Append the default include directories to the context.
 */
XKB_EXPORT int
xkb_context_include_path_append_default(struct xkb_context *ctx)
{
    const char *home;
    char *user_path;
    int err;
    int ret = 0;

    ret |= xkb_context_include_path_append(ctx, DFLT_XKB_CONFIG_ROOT);

    home = getenv("HOME");
    if (!home)
        return ret;
    err = asprintf(&user_path, "%s/.xkb", home);
    if (err <= 0)
        return ret;
    ret |= xkb_context_include_path_append(ctx, user_path);
    free(user_path);

    return ret;
}

/**
 * Remove all entries in the context's include path.
 */
XKB_EXPORT void
xkb_context_include_path_clear(struct xkb_context *ctx)
{
    char **path;

    darray_foreach(path, ctx->includes)
        free(*path);
    darray_free(ctx->includes);

    darray_foreach(path, ctx->failed_includes)
        free(*path);
    darray_free(ctx->failed_includes);
}

/**
 * xkb_context_include_path_clear() + xkb_context_include_path_append_default()
 */
XKB_EXPORT int
xkb_context_include_path_reset_defaults(struct xkb_context *ctx)
{
    xkb_context_include_path_clear(ctx);
    return xkb_context_include_path_append_default(ctx);
}

/**
 * Returns the number of entries in the context's include path.
 */
XKB_EXPORT unsigned int
xkb_context_num_include_paths(struct xkb_context *ctx)
{
    return darray_size(ctx->includes);
}

unsigned int
xkb_context_num_failed_include_paths(struct xkb_context *ctx)
{
    return darray_size(ctx->failed_includes);
}

/**
 * Returns the given entry in the context's include path, or NULL if an
 * invalid index is passed.
 */
XKB_EXPORT const char *
xkb_context_include_path_get(struct xkb_context *ctx, unsigned int idx)
{
    if (idx >= xkb_context_num_include_paths(ctx))
        return NULL;

    return darray_item(ctx->includes, idx);
}

const char *
xkb_context_failed_include_path_get(struct xkb_context *ctx,
                                    unsigned int idx)
{
    if (idx >= xkb_context_num_failed_include_paths(ctx))
        return NULL;

    return darray_item(ctx->failed_includes, idx);
}

/**
 * Take a new reference on the context.
 */
XKB_EXPORT struct xkb_context *
xkb_context_ref(struct xkb_context *ctx)
{
    ctx->refcnt++;
    return ctx;
}

/**
 * Drop an existing reference on the context, and free it if the refcnt is
 * now 0.
 */
XKB_EXPORT void
xkb_context_unref(struct xkb_context *ctx)
{
    if (!ctx || --ctx->refcnt > 0)
        return;

    xkb_context_include_path_clear(ctx);
    atom_table_free(ctx->atom_table);
    free(ctx);
}

static const char *
log_level_to_prefix(enum xkb_log_level level)
{
    switch (level) {
    case XKB_LOG_LEVEL_DEBUG:
        return "Debug:";
    case XKB_LOG_LEVEL_INFO:
        return "Info:";
    case XKB_LOG_LEVEL_WARNING:
        return "Warning:";
    case XKB_LOG_LEVEL_ERROR:
        return "Error:";
    case XKB_LOG_LEVEL_CRITICAL:
        return "Critical:";
    default:
        return NULL;
    }
}

ATTR_PRINTF(3, 0) static void
default_log_fn(struct xkb_context *ctx, enum xkb_log_level level,
               const char *fmt, va_list args)
{
    const char *prefix = log_level_to_prefix(level);

    if (prefix)
        fprintf(stderr, "%-10s", prefix);
    vfprintf(stderr, fmt, args);
}

static enum xkb_log_level
log_level(const char *level) {
    char *endptr;
    enum xkb_log_level lvl;

    errno = 0;
    lvl = strtol(level, &endptr, 10);
    if (errno == 0 && (endptr[0] == '\0' || isspace(endptr[0])))
        return lvl;
    if (istreq_prefix("crit", level))
        return XKB_LOG_LEVEL_CRITICAL;
    if (istreq_prefix("err", level))
        return XKB_LOG_LEVEL_ERROR;
    if (istreq_prefix("warn", level))
        return XKB_LOG_LEVEL_WARNING;
    if (istreq_prefix("info", level))
        return XKB_LOG_LEVEL_INFO;
    if (istreq_prefix("debug", level) || istreq_prefix("dbg", level))
        return XKB_LOG_LEVEL_DEBUG;

    return XKB_LOG_LEVEL_ERROR;
}

static int
log_verbosity(const char *verbosity) {
    char *endptr;
    int v;

    errno = 0;
    v = strtol(verbosity, &endptr, 10);
    if (errno == 0)
        return v;

    return 0;
}

#ifndef DEFAULT_XKB_VARIANT
#define DEFAULT_XKB_VARIANT NULL
#endif

#ifndef DEFAULT_XKB_OPTIONS
#define DEFAULT_XKB_OPTIONS NULL
#endif

/**
 * Create a new context.
 */
XKB_EXPORT struct xkb_context *
xkb_context_new(enum xkb_context_flags flags)
{
    const char *env;
    struct xkb_context *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->refcnt = 1;
    ctx->log_fn = default_log_fn;
    ctx->log_level = XKB_LOG_LEVEL_ERROR;
    ctx->log_verbosity = 0;

    /* Environment overwrites defaults. */
    env = getenv("XKB_LOG_LEVEL");
    if (env)
        xkb_context_set_log_level(ctx, log_level(env));

    env = getenv("XKB_LOG_VERBOSITY");
    if (env)
        xkb_context_set_log_verbosity(ctx, log_verbosity(env));

    if (!(flags & XKB_CONTEXT_NO_DEFAULT_INCLUDES) &&
        !xkb_context_include_path_append_default(ctx)) {
        log_err(ctx, "failed to add default include path %s\n",
                DFLT_XKB_CONFIG_ROOT);
        xkb_context_unref(ctx);
        return NULL;
    }

    ctx->use_environment_names = !(flags & XKB_CONTEXT_NO_ENVIRONMENT_NAMES);

    ctx->atom_table = atom_table_new();
    if (!ctx->atom_table) {
        xkb_context_unref(ctx);
        return NULL;
    }

    return ctx;
}

xkb_atom_t
xkb_atom_lookup(struct xkb_context *ctx, const char *string)
{
    return atom_lookup(ctx->atom_table, string);
}

xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string)
{
    return atom_intern(ctx->atom_table, string, false);
}

xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string)
{
    return atom_intern(ctx->atom_table, string, true);
}

char *
xkb_atom_strdup(struct xkb_context *ctx, xkb_atom_t atom)
{
    return atom_strdup(ctx->atom_table, atom);
}

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom)
{
    return atom_text(ctx->atom_table, atom);
}

void
xkb_log(struct xkb_context *ctx, enum xkb_log_level level,
        const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    ctx->log_fn(ctx, level, fmt, args);
    va_end(args);
}

XKB_EXPORT void
xkb_context_set_log_fn(struct xkb_context *ctx,
                       void (*log_fn)(struct xkb_context *ctx,
                                      enum xkb_log_level level,
                                      const char *fmt, va_list args))
{
    ctx->log_fn = (log_fn ? log_fn : default_log_fn);
}

XKB_EXPORT enum xkb_log_level
xkb_context_get_log_level(struct xkb_context *ctx)
{
    return ctx->log_level;
}

XKB_EXPORT void
xkb_context_set_log_level(struct xkb_context *ctx, enum xkb_log_level level)
{
    ctx->log_level = level;
}

XKB_EXPORT int
xkb_context_get_log_verbosity(struct xkb_context *ctx)
{
    return ctx->log_verbosity;
}

XKB_EXPORT void
xkb_context_set_log_verbosity(struct xkb_context *ctx, int verbosity)
{
    ctx->log_verbosity = verbosity;
}

XKB_EXPORT void *
xkb_context_get_user_data(struct xkb_context *ctx)
{
    if (ctx)
        return ctx->user_data;
    return NULL;
}

XKB_EXPORT void
xkb_context_set_user_data(struct xkb_context *ctx, void *user_data)
{
    ctx->user_data = user_data;
}

char *
xkb_context_get_buffer(struct xkb_context *ctx, size_t size)
{
    char *rtrn;

    if (size >= sizeof(ctx->text_buffer))
        return NULL;

    if (sizeof(ctx->text_buffer) - ctx->text_next <= size)
        ctx->text_next = 0;

    rtrn = &ctx->text_buffer[ctx->text_next];
    ctx->text_next += size;

    return rtrn;
}

const char *
xkb_context_get_default_rules(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = getenv("XKB_DEFAULT_RULES");

    return env ? env : DEFAULT_XKB_RULES;
}

const char *
xkb_context_get_default_model(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = getenv("XKB_DEFAULT_MODEL");

    return env ? env : DEFAULT_XKB_MODEL;
}

const char *
xkb_context_get_default_layout(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = getenv("XKB_DEFAULT_LAYOUT");

    return env ? env : DEFAULT_XKB_LAYOUT;
}

const char *
xkb_context_get_default_variant(struct xkb_context *ctx)
{
    const char *env = NULL;
    const char *layout = getenv("XKB_DEFAULT_VARIANT");

    /* We don't want to inherit the variant if they haven't also set a
     * layout, since they're so closely paired. */
    if (layout && ctx->use_environment_names)
        env = getenv("XKB_DEFAULT_VARIANT");

    return env ? env : DEFAULT_XKB_VARIANT;
}

const char *
xkb_context_get_default_options(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = getenv("XKB_DEFAULT_OPTIONS");

    return env ? env : DEFAULT_XKB_OPTIONS;
}
