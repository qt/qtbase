// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

export module Mod:Part;

import HelperMod;

// A Q_OBJECT class declared in an (exported) interface partition. Its signal takes a
// parameter type that comes from a different, external module that this partition imports.
export class PartObject : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

signals:
    void partSignal(ModuleHelper helper);
};
