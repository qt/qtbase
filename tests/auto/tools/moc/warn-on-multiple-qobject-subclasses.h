// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
