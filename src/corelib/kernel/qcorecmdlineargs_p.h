/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCORECMDLINEARGS_P_H
#define QCORECMDLINEARGS_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qstring.h"
#include "QtCore/qstringlist.h"

#if defined(Q_OS_WIN)
#  ifdef Q_OS_WIN32
#    include <qt_windows.h> // first to suppress min, max macros.
#    include <shlobj.h>
#  else
#    include "QtCore/qvector.h"
#    include <qt_windows.h>
#  endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WIN32)

static inline QStringList qWinCmdArgs(const QString &cmdLine)
{
    QStringList result;
    int size;
    if (wchar_t **argv = CommandLineToArgvW((const wchar_t *)cmdLine.utf16(), &size)) {
        result.reserve(size);
        wchar_t **argvEnd = argv + size;
        for (wchar_t **a = argv; a < argvEnd; ++a)
            result.append(QString::fromWCharArray(*a));
        LocalFree(argv);
    }
    return result;
}

#elif defined(Q_OS_WINRT) // Q_OS_WIN32

static inline QStringList qCmdLineArgs(int argc, char *argv[])
{
    QStringList args;
    for (int i = 0; i != argc; ++i)
        args += QString::fromLocal8Bit(argv[i]);
    return args;
}

#endif // Q_OS_WINRT

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCORECMDLINEARGS_WIN_P_H
