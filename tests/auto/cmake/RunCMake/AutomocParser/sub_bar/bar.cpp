// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bar.h"

class ChildSource : public QObject
{
    Q_OBJECT
};

ChildSource *makeChildSource()
{
    return new ChildSource;
}

#include "sub_bar/moc_bar.cpp"
#include "sub_bar/bar.moc"
