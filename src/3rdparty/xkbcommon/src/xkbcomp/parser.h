/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY__XKBCOMMON_XKBCOMMON_INTERNAL_STA_PARSER_H_INCLUDED
# define YY__XKBCOMMON_XKBCOMMON_INTERNAL_STA_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int  _xkbcommon_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END_OF_FILE = 0,
    ERROR_TOK = 255,
    XKB_KEYMAP = 1,
    XKB_KEYCODES = 2,
    XKB_TYPES = 3,
    XKB_SYMBOLS = 4,
    XKB_COMPATMAP = 5,
    XKB_GEOMETRY = 6,
    XKB_SEMANTICS = 7,
    XKB_LAYOUT = 8,
    INCLUDE = 10,
    OVERRIDE = 11,
    AUGMENT = 12,
    REPLACE = 13,
    ALTERNATE = 14,
    VIRTUAL_MODS = 20,
    TYPE = 21,
    INTERPRET = 22,
    ACTION_TOK = 23,
    KEY = 24,
    ALIAS = 25,
    GROUP = 26,
    MODIFIER_MAP = 27,
    INDICATOR = 28,
    SHAPE = 29,
    KEYS = 30,
    ROW = 31,
    SECTION = 32,
    OVERLAY = 33,
    TEXT = 34,
    OUTLINE = 35,
    SOLID = 36,
    LOGO = 37,
    VIRTUAL = 38,
    EQUALS = 40,
    PLUS = 41,
    MINUS = 42,
    DIVIDE = 43,
    TIMES = 44,
    OBRACE = 45,
    CBRACE = 46,
    OPAREN = 47,
    CPAREN = 48,
    OBRACKET = 49,
    CBRACKET = 50,
    DOT = 51,
    COMMA = 52,
    SEMI = 53,
    EXCLAM = 54,
    INVERT = 55,
    STRING = 60,
    INTEGER = 61,
    FLOAT = 62,
    IDENT = 63,
    KEYNAME = 64,
    PARTIAL = 70,
    DEFAULT = 71,
    HIDDEN = 72,
    ALPHANUMERIC_KEYS = 73,
    MODIFIER_KEYS = 74,
    KEYPAD_KEYS = 75,
    FUNCTION_KEYS = 76,
    ALTERNATE_GROUP = 77
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 162 "../src/xkbcomp/parser.y" /* yacc.c:1909  */

        int              ival;
        int64_t          num;
        enum xkb_file_type file_type;
        char            *str;
        xkb_atom_t      atom;
        enum merge_mode merge;
        enum xkb_map_flags mapFlags;
        xkb_keysym_t    keysym;
        ParseCommon     *any;
        ExprDef         *expr;
        VarDef          *var;
        VModDef         *vmod;
        InterpDef       *interp;
        KeyTypeDef      *keyType;
        SymbolsDef      *syms;
        ModMapDef       *modMask;
        GroupCompatDef  *groupCompat;
        LedMapDef       *ledMap;
        LedNameDef      *ledName;
        KeycodeDef      *keyCode;
        KeyAliasDef     *keyAlias;
        void            *geom;
        XkbFile         *file;

#line 146 "xkbcommon-internal@sta/parser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int  _xkbcommon_parse (struct parser_param *param);

#endif /* !YY__XKBCOMMON_XKBCOMMON_INTERNAL_STA_PARSER_H_INCLUDED  */
