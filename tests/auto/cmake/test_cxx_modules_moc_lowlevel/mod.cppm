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

// InternalObject isn't exported, so it can't be named from outside the module; this is
// implemented (importing the :Internal partition) in mod-test-helpers.cpp instead, since
// importing an implementation partition from the module interface unit itself isn't
// recommended.
export bool testInternalObject();
