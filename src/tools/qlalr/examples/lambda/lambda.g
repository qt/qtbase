-- Copyright (C) 2016 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

-- lambda calculus

%decl lambda.h

%token LPAREN
%token RPAREN
%token ID
%token FUN
%token DOT

%nonassoc SHIFT_THERE
%nonassoc LPAREN RPAREN ID FUN DOT
%nonassoc REDUCE_HERE

%start Expr

/:
enum {
:/


Expr ::= ID %prec SHIFT_THERE ;
/:  Symbol = $rule_number,
:/

Expr ::= LPAREN Expr RPAREN %prec SHIFT_THERE  ;
/:  SubExpression = $rule_number,
:/

Expr ::= Expr Expr %prec REDUCE_HERE ;
/:  Appl = $rule_number,
:/

Expr ::= FUN ID DOT Expr %prec SHIFT_THERE ;
/:  Abstr = $rule_number,
:/

/:};
:/

