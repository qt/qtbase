// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJSONPARSEERROR_H
#define QJSONPARSEERROR_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>

QT_BEGIN_NAMESPACE

class QString;

struct Q_CORE_EXPORT QJsonParseError
{
    enum ParseError {
        NoError = 0,
        UnterminatedObject,
        MissingNameSeparator,
        UnterminatedArray,
        MissingValueSeparator,
        IllegalValue,
        TerminationByNumber,
        IllegalNumber,
        IllegalEscapeSequence,
        IllegalUTF8String,
        UnterminatedString,
        MissingObject,
        DeepNesting,
        DocumentTooLarge,
        GarbageAtEnd
    };

    QString errorString() const;

    int offset = -1;
    ParseError error = NoError;
};

QT_END_NAMESPACE

#endif // QJSONPARSEERROR_H
