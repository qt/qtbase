// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef OLDSTYLE_CASTS_H
#define OLDSTYLE_CASTS_H
#include <QtCore/qobject.h>

class OldStyleCast: public QObject
{
    Q_OBJECT
public:


signals:

public slots:
    inline void foo() {}
    inline int bar(int, int*, const int *, volatile int *, const int * volatile *) { return 0; }
    inline void slot(int, QObject * const) {}
};

#endif // OLDSTYLE_CASTS_H
