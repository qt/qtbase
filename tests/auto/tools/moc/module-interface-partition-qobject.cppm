// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

export module Module.InterfacePartitionQObject:Part;

export class PartValid : public QObject
{
    Q_OBJECT

signals:
    void partSignal();
};
