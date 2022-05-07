/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSYSTEMERROR_P_H
#define QSYSTEMERROR_P_H

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
#include <qstring.h>

QT_BEGIN_NAMESPACE

class QSystemError
{
public:
    enum ErrorScope
    {
        NoError,
        StandardLibraryError,
        NativeError
    };

    constexpr explicit QSystemError(int error, ErrorScope scope)
        : errorCode(error), errorScope(scope)
    {
    }
    constexpr QSystemError() = default;

    QString toString() const { return string(errorScope, errorCode); }
    constexpr ErrorScope scope() const { return errorScope; }
    constexpr int error() const { return errorCode; }

    static Q_CORE_EXPORT QString string(ErrorScope errorScope, int errorCode);
    static Q_CORE_EXPORT QString stdString(int errorCode = -1);
#ifdef Q_OS_WIN
    static Q_CORE_EXPORT QString windowsString(int errorCode = -1);
#endif

    // data members
    int errorCode = 0;
    ErrorScope errorScope = NoError;
};

QT_END_NAMESPACE

#endif // QSYSTEMERROR_P_H
