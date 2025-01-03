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

#ifdef Q_CC_MSVC
#define ErrorFormatString "%s(%d:%d): "
#else
#define ErrorFormatString "%s:%d:%d: "
#endif

static void defaultErrorMsg(const QByteArray &fileName, const Symbol &sym)
{
    fprintf(stderr, ErrorFormatString "error: Parse error at \"%s\"\n",
            fileName.constData(), sym.lineNum, 1, sym.lexem().data());
}

void Parser::error(const Symbol &sym)
{
    defaultErrorMsg(currentFilenames.top(), sym);
    exit(EXIT_FAILURE);
}

void Parser::error(const char *msg) {
    if (msg || error_msg)
        fprintf(stderr, ErrorFormatString "error: %s\n",
                 currentFilenames.top().constData(), symbol().lineNum, 1, msg?msg:error_msg);
    else
        defaultErrorMsg(currentFilenames.top(), symbol());
    exit(EXIT_FAILURE);
}

void Parser::warning(const char *msg) {
    if (displayWarnings && msg)
        fprintf(stderr, ErrorFormatString "warning: %s\n",
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), 1, msg);
}

void Parser::note(const char *msg) {
    if (displayNotes && msg)
        fprintf(stderr, ErrorFormatString "note: %s\n",
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), 1, msg);
}

QT_END_NAMESPACE
