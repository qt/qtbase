// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bar.h"

class ParentSource : public QObject
{
    Q_OBJECT
};

ParentSource *makeParentSource()
{
    return new ParentSource;
}

#include "moc_bar.cpp"
#include "bar.moc"
