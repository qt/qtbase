// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

module;

#include <QObject>

#include <cstring>

module Mod;
import :Internal;

// Exercises InternalObject's (moc-generated) signal/slot machinery on behalf of
// testInternalObject(), declared in mod.cppm, since InternalObject itself isn't exported and
// therefore can't be named from outside the module.
bool testInternalObject()
{
    InternalObject internal;
    bool received = false;
    QObject::connect(&internal, &InternalObject::internalSignal, [&received] { received = true; });
    emit internal.internalSignal();
    return received
            && std::strcmp(InternalObject::staticMetaObject.className(), "InternalObject") == 0;
}
