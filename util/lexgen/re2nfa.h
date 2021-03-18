/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef RE2NFA_H
#define RE2NFA_H

#include "nfa.h"
#include <QSet>

class RE2NFA
{
public:
    RE2NFA(const QMap<QString, NFA> &macros, const QSet<InputType> &maxInputSet, Qt::CaseSensitivity cs);

    NFA parse(const QString &expression, int *errorColumn = 0);

private:
    NFA parseExpr();
    NFA parseBranch();
    NFA parsePiece();
    NFA parseAtom();
    NFA parseMaybeQuantifier(const NFA &nfa);
    NFA parseSet();
    NFA parseSet2();

    NFA createCharNFA();

private:
    friend class RegExpTokenizer;

    enum Token {
        TOK_INVALID,
        TOK_STRING,
        TOK_LBRACE,   // {
        TOK_RBRACE,   // }
        TOK_LBRACKET, // [
        TOK_RBRACKET, // ]
        TOK_LPAREN,   // (
        TOK_RPAREN,   // )
        TOK_COMMA,
        TOK_STAR,
        TOK_OR,
        TOK_QUESTION,
        TOK_DOT,
        TOK_PLUS,
        TOK_SEQUENCE,
        TOK_QUOTED_STRING
    };

    struct Symbol
    {
        inline Symbol() : token(TOK_INVALID), column(-1) {}
        inline Symbol(Token t, const QString &l = QString()) : token(t), lexem(l), column(-1) {}
        Token token;
        QString lexem;
        int column;
    };

    inline bool hasNext() const { return index < symbols.count(); }
    inline Token next() { return symbols.at(index++).token; }
    bool next(Token t);
    bool test(Token t);
    inline void prev() { index--; }
    inline const Symbol &symbol() const { return symbols.at(index - 1); }
    QString lexemUntil(Token t);

    void tokenize(const QString &input);

    QMap<QString, NFA> macros;
    QVector<Symbol> symbols;
    int index;
    int errorColumn;
    const QSet<InputType> maxInputSet;
    Qt::CaseSensitivity caseSensitivity;
};

#endif // RE2NFA_H

