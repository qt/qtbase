// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QList<Symbol> symbols;
    int index;
    int errorColumn;
    const QSet<InputType> maxInputSet;
    Qt::CaseSensitivity caseSensitivity;
};

#endif // RE2NFA_H

