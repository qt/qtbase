// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BACKSLASH_NEWLINES_H
#define BACKSLASH_NEWLINES_H

#include <QObject>

const int blackslashNewlinesDummy = 0

#define value 0\
1

;

class BackslashNewlines : public QObject
{
    Q_OBJECT
public slots:
#if value
    void works() {}
#else
    void buggy() {}
#endif
};

#undef value

#endif // BACKSLASH_NEWLINES_H

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wcomment")
QT_WARNING_DISABLE_GCC("-Wcomment")

// ends with \\\r should not make moc crash (QTBUG-53441) (no new lines on purpose!!) \

QT_WARNING_POP
