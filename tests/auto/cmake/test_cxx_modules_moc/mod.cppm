// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

#include "helper.h"

export module Mod;

export import :Part;

// A Q_OBJECT class declared in the primary module interface unit. Its signal takes a
// parameter type that comes from an ordinary header, standing in for an external,
// non-modularized dependency.
export class PrimaryObject : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

signals:
    void primarySignal(HeaderHelper helper);
};
