// Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhooks_p.h"

QT_BEGIN_NAMESPACE

// Only add to the end, and bump version if you do.
quintptr Q_CORE_EXPORT qtHookData[] = {
    3, // hook data version
    QHooks::LastHookIndex, // size of qtHookData
    QT_VERSION,

    // AddQObject, void(*)(QObject*), called for every constructed QObject
    // Note: this is called from the QObject constructor, ie. the sub-class
    // constructors haven't run yet.
    0,

    // RemoveQObject, void(*)(QObject*), called for every destructed QObject
    // Note: this is called from the QObject destructor, ie. the object
    // you get as an argument is already largely invalid.
    0,

    // Startup, void(*)(), called once QCoreApplication is operational
    0,

    // TypeInformationVersion, an integral value, bumped whenever private
    // object sizes or member offsets that are used in Qt Creator's
    // data structure "pretty printing" change.
    //
    // The required sizes and offsets are tested in tests/auto/other/toolsupport.
    // When this fails and the change was intentional, adjust the test and
    // adjust this value here.
    22,
};

static_assert(QHooks::LastHookIndex == sizeof(qtHookData) / sizeof(qtHookData[0]));

QT_END_NAMESPACE

