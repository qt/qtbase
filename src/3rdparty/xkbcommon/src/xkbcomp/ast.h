/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 */

#ifndef XKBCOMP_AST_H
#define XKBCOMP_AST_H

enum xkb_file_type {
    /* Component files, by order of compilation. */
    FILE_TYPE_KEYCODES = 0,
    FILE_TYPE_TYPES = 1,
    FILE_TYPE_COMPAT = 2,
    FILE_TYPE_SYMBOLS = 3,
    /* Geometry is not compiled any more. */
    FILE_TYPE_GEOMETRY = 4,

    /* A top level file which includes the above files. */
    FILE_TYPE_KEYMAP,

/* File types which must be found in a keymap file. */
#define FIRST_KEYMAP_FILE_TYPE FILE_TYPE_KEYCODES
#define LAST_KEYMAP_FILE_TYPE  FILE_TYPE_SYMBOLS

    /* This one doesn't mix with the others, but useful here as well. */
    FILE_TYPE_RULES,

    _FILE_TYPE_NUM_ENTRIES
};

enum stmt_type {
    STMT_UNKNOWN = 0,
    STMT_INCLUDE,
    STMT_KEYCODE,
    STMT_ALIAS,
    STMT_EXPR,
    STMT_VAR,
    STMT_TYPE,
    STMT_INTERP,
    STMT_VMOD,
    STMT_SYMBOLS,
    STMT_MODMAP,
    STMT_GROUP_COMPAT,
    STMT_LED_MAP,
    STMT_LED_NAME,

    _STMT_NUM_VALUES
};

enum expr_value_type {
    EXPR_TYPE_UNKNOWN = 0,
    EXPR_TYPE_BOOLEAN,
    EXPR_TYPE_INT,
    EXPR_TYPE_STRING,
    EXPR_TYPE_ACTION,
    EXPR_TYPE_KEYNAME,
    EXPR_TYPE_SYMBOLS,

    _EXPR_TYPE_NUM_VALUES
};

enum expr_op_type {
    EXPR_VALUE,
    EXPR_IDENT,
    EXPR_ACTION_DECL,
    EXPR_FIELD_REF,
    EXPR_ARRAY_REF,
    EXPR_KEYSYM_LIST,
    EXPR_ACTION_LIST,
    EXPR_ADD,
    EXPR_SUBTRACT,
    EXPR_MULTIPLY,
    EXPR_DIVIDE,
    EXPR_ASSIGN,
    EXPR_NOT,
    EXPR_NEGATE,
    EXPR_INVERT,
    EXPR_UNARY_PLUS,

    _EXPR_NUM_VALUES
};

enum merge_mode {
    MERGE_DEFAULT,
    MERGE_AUGMENT,
    MERGE_OVERRIDE,
    MERGE_REPLACE,
};

const char *
xkb_file_type_to_string(enum xkb_file_type type);

const char *
stmt_type_to_string(enum stmt_type type);

const char *
expr_op_type_to_string(enum expr_op_type type);

const char *
expr_value_type_to_string(enum expr_value_type type);

typedef struct _ParseCommon {
    enum stmt_type type;
    struct _ParseCommon *next;
} ParseCommon;

typedef struct _IncludeStmt {
    ParseCommon common;
    enum merge_mode merge;
    char *stmt;
    char *file;
    char *map;
    char *modifier;
    struct _IncludeStmt *next_incl;
} IncludeStmt;

typedef struct _Expr {
    ParseCommon common;
    enum expr_op_type op;
    enum expr_value_type value_type;
    union {
        struct {
            struct _Expr *left;
            struct _Expr *right;
        } binary;
        struct {
            xkb_atom_t element;
            xkb_atom_t field;
        } field;
        struct {
            xkb_atom_t element;
            xkb_atom_t field;
            struct _Expr *entry;
        } array;
        struct {
            xkb_atom_t name;
            struct _Expr *args;
        } action;
        struct {
            darray(char *) syms;
            darray(int) symsMapIndex;
            darray(unsigned int) symsNumEntries;
        } list;
        struct _Expr *child;
        xkb_atom_t str;
        unsigned uval;
        int ival;
        xkb_atom_t keyName;
    } value;
} ExprDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    ExprDef *name;
    ExprDef *value;
} VarDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    ExprDef *value;
} VModDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    int64_t value;
} KeycodeDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t alias;
    xkb_atom_t real;
} KeyAliasDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    VarDef *body;
} KeyTypeDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t keyName;
    ExprDef *symbols;
} SymbolsDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t modifier;
    ExprDef *keys;
} ModMapDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    int group;
    ExprDef *def;
} GroupCompatDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    char *sym;
    ExprDef *match;
    VarDef *def;
} InterpDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    int ndx;
    ExprDef *name;
    bool virtual;
} LedNameDef;

typedef struct {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    VarDef *body;
} LedMapDef;

enum xkb_map_flags {
    MAP_IS_DEFAULT = (1 << 0),
    MAP_IS_PARTIAL = (1 << 1),
    MAP_IS_HIDDEN = (1 << 2),
    MAP_HAS_ALPHANUMERIC = (1 << 3),
    MAP_HAS_MODIFIER = (1 << 4),
    MAP_HAS_KEYPAD = (1 << 5),
    MAP_HAS_FN = (1 << 6),
    MAP_IS_ALTGR = (1 << 7),
};

typedef struct {
    ParseCommon common;
    enum xkb_file_type file_type;
    char *topName;
    char *name;
    ParseCommon *defs;
    enum xkb_map_flags flags;
} XkbFile;

#endif
