// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PONG_H
#define PONG_H

#include <QtCore/QObject>

class Pong: public QObject
{
    Q_OBJECT
public slots:
    Q_SCRIPTABLE QString ping(const QString &arg);
};

#endif
