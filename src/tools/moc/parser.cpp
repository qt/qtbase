/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
******************************************************************************/

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

void Parser::error(int rollback) {
    index -= rollback;
    error();
}
void Parser::error(const char *msg) {
    if (msg || error_msg)
        fprintf(stderr, ErrorFormatString "error: %s\n",
                 currentFilenames.top().constData(), symbol().lineNum, 1, msg?msg:error_msg);
    else
        fprintf(stderr, ErrorFormatString "error: Parse error at \"%s\"\n",
                 currentFilenames.top().constData(), symbol().lineNum, 1, symbol().lexem().data());
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
