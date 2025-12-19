// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VIRTUALANDOVERRIDE_H
#define VIRTUALANDOVERRIDE_H

#include <qobject.h>

class WithVirtual : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int p MEMBER p VIRTUAL)

    int p;
};

class WithOverride : public WithVirtual
{
    Q_OBJECT
    Q_PROPERTY(QString p MEMBER p OVERRIDE)

    QString p;
};

#endif // VIRTUALANDOVERRIDE_H
