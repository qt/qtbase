// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QObject>

class MetaType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int test READ test)
    int test() { return 0; }
};
