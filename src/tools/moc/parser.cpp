// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#ifdef USE_LEXEM_STORE
Symbol::LexemStore Symbol::lexemStore;
#endif

static const char *error_msg = nullptr;

/*! \internal
    Base implementation for printing diagnostic messages.

    For example:
        "/path/to/file:line:column: error: %s\n"
        '%s' is replaced by \a msg. (Currently "column" is always 1).

    If sym.lineNum is -1, the line and column parts aren't printed:
        "/path/to/file: error: %s\n"

    \a formatStringSuffix specifies the type of the message e.g.:
        "error: %s\n"
        "warning: %s\n"
        "note: %s\n"
        "Parse error at %s\n" (from defaultErrorMsg())
*/
void Parser::printMsg(QByteArrayView formatStringSuffix, QByteArrayView msg, const Symbol &sym)
{
    if (sym.lineNum != -1) {
#ifdef Q_CC_MSVC
        QByteArray formatString = "%s(%d:%d): " + formatStringSuffix;
#else
        QByteArray formatString = "%s:%d:%d: " + formatStringSuffix;
#endif
        fprintf(stderr, formatString.constData(),
                currentFilenames.top().constData(), sym.lineNum, 1, msg.data());
    } else {
        QByteArray formatString = "%s: " + formatStringSuffix;
        fprintf(stderr, formatString.constData(),
                currentFilenames.top().constData(), msg.data());
    }
}

void Parser::defaultErrorMsg(const Symbol &sym)
{
    if (sym.lineNum != -1)
        printMsg("error: Parse error at \"%s\"\n", sym.lexem().data(), sym);
    else
        printMsg("error: could not parse file\n", "", sym);
}

void Parser::error(const Symbol &sym)
{
    defaultErrorMsg(sym);
    exit(EXIT_FAILURE);
}

void Parser::error(const char *msg)
{
    if (msg || error_msg)
        printMsg("error: %s\n",
                 msg ? msg : error_msg,
                 index > 0 ? symbol() : Symbol{});
    else
        defaultErrorMsg(symbol());

    exit(EXIT_FAILURE);
}

void Parser::warning(const char *msg) {
    if (displayWarnings && msg)
        printMsg("warning: %s\n", msg, index > 0 ? symbol() : Symbol{});
}

void Parser::note(const char *msg) {
    if (displayNotes && msg)
        printMsg("note: %s\n", msg, index > 0 ? symbol() : Symbol{});
}

QT_END_NAMESPACE
