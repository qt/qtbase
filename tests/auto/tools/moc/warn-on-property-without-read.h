// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WARN_ON_PROPERTY_WITHOUT_READ_H
#define WARN_ON_PROPERTY_WITHOUT_READ_H
#include <QObject>

class ClassWithPropertyWithoutREAD : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo)
    Q_PROPERTY(int bar READ bar)
};
#endif // WARN_ON_PROPERTY_WITHOUT_READ_H
