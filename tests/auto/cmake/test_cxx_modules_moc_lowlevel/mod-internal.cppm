// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

module Mod:Internal;

// A Q_OBJECT class declared in an internal (module-private) partition. It is not
// exported and therefore only visible to other partitions and implementation units
// of this module, not to importers of Mod.
class InternalObject : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

signals:
    void internalSignal();
};
