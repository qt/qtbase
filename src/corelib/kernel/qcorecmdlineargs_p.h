// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#endif // Q_OS_WIN32

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCORECMDLINEARGS_WIN_P_H
