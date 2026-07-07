// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>

class ImplA : public QObject
{
    Q_OBJECT
};

ImplA *makeImplA()
{
    return new ImplA;
}

#include "foo.moc"
