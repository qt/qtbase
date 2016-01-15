/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

QT_BEGIN_NAMESPACE

#include <QtCore/qglobal.h>

#ifdef Q_OS_WIN32
   #include <QtCore/qt_windows.h>
#else
    typedef void* HKEY;
#endif

#include <QtCore/qstring.h>

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

