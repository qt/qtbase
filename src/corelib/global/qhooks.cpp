/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    15
};

Q_STATIC_ASSERT(QHooks::LastHookIndex == sizeof(qtHookData) / sizeof(qtHookData[0]));

QT_END_NAMESPACE

