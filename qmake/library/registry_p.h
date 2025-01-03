// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QT_WINDOWS_REGISTRY_H
#define QT_WINDOWS_REGISTRY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifdef Q_OS_WIN32
   #include <QtCore/qt_windows.h>
#else
    typedef void* HKEY;
#endif

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

/**
 * Read a value from the Windows registry.
 *
 * If the key is not found, or the registry cannot be accessed (for example
 * if this code is compiled for a platform other than Windows), a null
 * string is returned.
 *
 * 32-bit code reads from the registry's 32 bit view (Wow6432Node),
 * 64 bit code reads from the 64 bit view.
 * Pass KEY_WOW64_32KEY to access the 32 bit view regardless of the
 * application's architecture, KEY_WOW64_64KEY respectively.
 */
QString qt_readRegistryKey(HKEY parentHandle, const QString &rSubkey,
                           unsigned long options = 0);

QT_END_NAMESPACE

#endif // QT_WINDOWS_REGISTRY_H

