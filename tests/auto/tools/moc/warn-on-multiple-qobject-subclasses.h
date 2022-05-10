// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WARN_ON_MULTIPLE_QOBJECT_SUBCLASSES_H
#define WARN_ON_MULTIPLE_QOBJECT_SUBCLASSES_H

#include <QtGui>

class Foo : public QObject
{
    Q_OBJECT
public:
};

class Bar : public QWindow, public Foo
{
    Q_OBJECT
};


#endif // WARN_ON_MULTIPLE_QOBJECT_SUBCLASSES_H
