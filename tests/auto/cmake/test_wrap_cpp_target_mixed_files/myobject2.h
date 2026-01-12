// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QObject>

class MyObject2 : public QObject
{
    Q_OBJECT
public:
    explicit MyObject2(QObject *parent = nullptr);
};
