// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "foo.h"

class SourceType : public QObject
{
    Q_OBJECT
};

SourceType *makeSourceType()
{
    return new SourceType;
}

#include "moc_foo.cpp"
#include "foo.moc"
