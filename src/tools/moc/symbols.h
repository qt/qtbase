// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "token.h"
#include <qdebug.h>
#include <qhashfunctions.h>
#include <qlist.h>
#include <qstack.h>
#include <qstring.h>
#include <qset.h>

QT_BEGIN_NAMESPACE

//#define USE_LEXEM_STORE

struct SubArray
{
    inline SubArray() = default;
    inline SubArray(const QByteArray &a):array(a),from(0), len(a.size()){}
    inline SubArray(const char *s):array(s),from(0) { len = array.size(); }
    SubArray(const QByteArray &a, qsizetype from, qsizetype len)
        : array(a), from(from), len(len)
    {
    }
    QByteArray array;
    qsizetype from = 0;
    qsizetype len = -1;
    inline bool operator==(const SubArray &other) const {
        if (len != other.len)
            return false;
        const auto begin = array.cbegin() + from;
        const auto end = begin + len;
        const auto other_begin = other.array.cbegin() + other.from;
        return std::equal(begin, end, other_begin);
    }
};

inline size_t qHash(const SubArray &key, size_t seed = 0)
{
    return qHash(QLatin1StringView(key.array.constData() + key.from, key.len), seed);
}


struct Symbol
{

#ifdef USE_LEXEM_STORE
    typedef QHash<SubArray, QHashDummyValue> LexemStore;
    static LexemStore lexemStore;

    inline Symbol() : lineNum(-1),token(NOTOKEN){}
    inline Symbol(int lineNum, Token token):
        lineNum(lineNum), token(token){}
    inline Symbol(int lineNum, Token token, const QByteArray &lexem):
        lineNum(lineNum), token(token),lex(lexem){}
    inline Symbol(int lineNum, Token token, const QByteArray &lexem, int from, int len):
        lineNum(lineNum), token(token){
        LexemStore::const_iterator it = lexemStore.constFind(SubArray(lexem, from, len));

        if (it != lexemStore.constEnd()) {
            lex = it.key().array;
        } else {
            lex = lexem.mid(from, len);
            lexemStore.insert(lex, QHashDummyValue());
        }
    }
    int lineNum;
    Token token;
    inline QByteArray unquotedLexem() const { return lex.mid(1, lex.length()-2); }
    inline QByteArray lexem() const { return lex; }
    inline operator QByteArray() const { return lex; }
    QByteArray lex;

#else

    inline Symbol() = default;
    inline Symbol(int lineNum, Token token) : lineNum(lineNum), token(token) { }
    inline Symbol(int lineNum, Token token, const QByteArray &lexem)
        : lineNum(lineNum), token(token), lex(lexem), len(lex.size())
    {
    }
    Symbol(int lineNum, Token token, const QByteArray &lexem, qsizetype from, qsizetype len)
        : lineNum(lineNum), token(token), lex(lexem), from(from), len(len)
    {
    }
    int lineNum = -1;
    Token token = NOTOKEN;
    inline QByteArray lexem() const { return lex.mid(from, len); }
    inline QByteArray unquotedLexem() const { return lex.mid(from+1, len-2); }
    inline operator SubArray() const { return SubArray(lex, from, len); }
    bool operator==(const Symbol& o) const
    {
        return SubArray(lex, from, len) == SubArray(o.lex, o.from, o.len);
    }
    QByteArray lex;
    qsizetype from = 0;
    qsizetype len = -1;

#endif
};
Q_DECLARE_TYPEINFO(Symbol, Q_RELOCATABLE_TYPE);

typedef QList<Symbol> Symbols;

struct SafeSymbols {
    Symbols symbols;
    QByteArray expandedMacro;
    QSet<QByteArray> excludedSymbols;
    qsizetype index;
};
Q_DECLARE_TYPEINFO(SafeSymbols, Q_RELOCATABLE_TYPE);

class SymbolStack : public QStack<SafeSymbols>
{
public:
    inline bool hasNext() {
        while (!isEmpty() && top().index >= top().symbols.size())
            pop();
        return !isEmpty();
    }
    inline Token next() {
        while (!isEmpty() && top().index >= top().symbols.size())
            pop();
        if (isEmpty())
            return NOTOKEN;
        return top().symbols.at(top().index++).token;
    }
    bool test(Token);
    inline const Symbol &symbol() const { return top().symbols.at(top().index-1); }
    inline Token token() { return symbol().token; }
    inline QByteArray lexem() const { return symbol().lexem(); }
    inline QByteArray unquotedLexem() { return symbol().unquotedLexem(); }

    bool dontReplaceSymbol(const QByteArray &name) const;
    QSet<QByteArray> excludeSymbols() const;
};

inline bool SymbolStack::test(Token token)
{
    qsizetype stackPos = size() - 1;
    while (stackPos >= 0 && at(stackPos).index >= at(stackPos).symbols.size())
        --stackPos;
    if (stackPos < 0)
        return false;
    if (at(stackPos).symbols.at(at(stackPos).index).token == token) {
        next();
        return true;
    }
    return false;
}

inline bool SymbolStack::dontReplaceSymbol(const QByteArray &name) const
{
    auto matchesName = [&name](const SafeSymbols &sf) {
        return name == sf.expandedMacro || sf.excludedSymbols.contains(name);
    };
    return std::any_of(cbegin(), cend(), matchesName);
}

inline QSet<QByteArray> SymbolStack::excludeSymbols() const
{
    QSet<QByteArray> set;
    for (const SafeSymbols &sf : *this) {
        set << sf.expandedMacro;
        set += sf.excludedSymbols;
    }
    return set;
}

QT_END_NAMESPACE

#endif // SYMBOLS_H
