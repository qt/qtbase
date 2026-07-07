// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>

class Impl : public QObject
{
    Q_OBJECT
};

Impl *makeImpl()
{
    return new Impl;
}

#include "foo.moc"
