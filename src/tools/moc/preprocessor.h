// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser.h"
#include <qlist.h>
#include <qset.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

struct Macro
{
    Macro() : isFunction(false), isVariadic(false) {}
    bool isFunction;
    bool isVariadic;
    Symbols arguments;
    Symbols symbols;
};

typedef SubArray MacroName;
typedef QHash<MacroName, Macro> Macros;

class QFile;

// Cached result of resolving an include: the canonical path of the resolved
// file (null if not found) and the index into the include search list where it
// was found (-1 if resolved relative to the including file, or not found). The
// index lets #include_next resume the search *after* the directory the current
// file was found in.
struct IncludeResolution
{
    QByteArray path;
    qsizetype foundIndex = -1;
};

class Preprocessor : public Parser
{
public:
    Preprocessor(){}
    static bool preprocessOnly;
    QList<QByteArray> frameworks;
    QSet<QByteArray> preprocessedIncludes;
    QHash<QByteArray, IncludeResolution> nonlocalIncludePathResolutionCache;
    Macros macros;
    QByteArray resolveInclude(const QByteArray &filename, const QByteArray &relativeTo,
                              qsizetype *foundIndex = nullptr);
    QByteArray resolveIncludeNext(const QByteArray &filename, qsizetype startIndex,
                                  qsizetype *foundIndex = nullptr);
    Symbols preprocessed(const QByteArray &filename, QFile *device);

    void parseDefineArguments(Macro *m);

    void skipUntilEndif();
    bool skipBranch();

    void substituteUntilNewline(Symbols &substituted);
    static Symbols macroExpandIdentifier(Preprocessor *that, SymbolStack &symbols, int lineNum, QByteArray *macroName);
    static void macroExpand(Symbols *into, Preprocessor *that, const Symbols &toExpand,
                            qsizetype &index, int lineNum, bool one,
                            const QSet<QByteArray> &excludeSymbols = QSet<QByteArray>());

    int evaluateCondition();

    enum TokenizeMode { TokenizeCpp, TokenizePreprocessor, PreparePreprocessorStatement, TokenizePreprocessorStatement, TokenizeInclude, PrepareDefine, TokenizeDefine };
    static Symbols tokenize(const QByteArray &input, int lineNum = 1, TokenizeMode mode = TokenizeCpp);

    void setDebugIncludes(bool value);

private:
    void until(Token);

    void preprocess(const QByteArray &filename, Symbols &preprocessed, qsizetype includeDirIndex = -1);
    // Parallel to Parser::currentFilenames: for each file currently being
    // preprocessed, the index into the include search list where it was found
    // (-1 if not found via the list). Used to implement #include_next.
    std::stack<qsizetype, QList<qsizetype>> currentIncludeDirIndex;
    // The index to start searching from for an #include_next / __has_include_next
    // in the current file: one past the directory it was found in, or the start
    // of the path when it was not found via the include path.
    qsizetype includeNextStartIndex();
    bool debugIncludes = false;
};

QT_END_NAMESPACE

#endif // PREPROCESSOR_H
