// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ESCAPES_IN_STRING_LITERALS_H
#define ESCAPES_IN_STRING_LITERALS_H
#include <QObject>

class StringLiterals: public QObject
{
        Q_OBJECT
        Q_CLASSINFO("Test", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\x53")
        Q_CLASSINFO("Test2", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\123")
        Q_CLASSINFO("Test3", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nb")
};
#endif // ESCAPES_IN_STRING_LITERALS_H
