// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>

class ImplB : public QObject
{
    Q_OBJECT
};

ImplB *makeImplB()
{
    return new ImplB;
}

#include "dot_moc_same_name/sub/foo.moc"
