// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

export module Module.PrimaryInterfaceQObject;

export class PrimaryValid : public QObject
{
    Q_OBJECT

signals:
    void primarySignal();
};
