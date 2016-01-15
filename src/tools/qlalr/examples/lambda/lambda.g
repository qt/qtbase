----------------------------------------------------------------------------
--
-- Copyright (C) 2016 The Qt Company Ltd.
-- Contact: https://www.qt.io/licensing/
--
-- This file is part of the QtCore module of the Qt Toolkit.
--
-- $QT_BEGIN_LICENSE:GPL-EXCEPT$
-- Commercial License Usage
-- Licensees holding valid commercial Qt licenses may use this file in
-- accordance with the commercial license agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and The Qt Company. For licensing terms
-- and conditions see https://www.qt.io/terms-conditions. For further
-- information use the contact form at https://www.qt.io/contact-us.
--
-- GNU General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU
-- General Public License version 3 as published by the Free Software
-- Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
-- included in the packaging of this file. Please review the following
-- information to ensure the GNU General Public License requirements will
-- be met: https://www.gnu.org/licenses/gpl-3.0.html.
--
-- $QT_END_LICENSE$
--
----------------------------------------------------------------------------

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

