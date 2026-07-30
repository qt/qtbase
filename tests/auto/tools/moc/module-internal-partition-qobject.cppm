// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

module Module.InternalPartitionQObject:Part;

class InternalValid : public QObject
{
    Q_OBJECT

signals:
    void internalSignal();
};
