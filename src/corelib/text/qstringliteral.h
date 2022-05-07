/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2020 Intel Corporation.
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

#ifndef QSTRINGLITERAL_H
#define QSTRINGLITERAL_H

#include <QtCore/qarraydata.h>
#include <QtCore/qarraydatapointer.h>

#if 0
#pragma qt_class(QStringLiteral)
#endif

QT_BEGIN_NAMESPACE

// all our supported compilers support Unicode string literals,
// even if their Q_COMPILER_UNICODE_STRING has been revoked due
// to lacking stdlib support. But QStringLiteral only needs the
// core language feature, so just use u"" here unconditionally:

#define QT_UNICODE_LITERAL(str) u"" str

using QStringPrivate = QArrayDataPointer<char16_t>;

namespace QtPrivate {
template <qsizetype N>
static Q_ALWAYS_INLINE QStringPrivate qMakeStringPrivate(const char16_t (&literal)[N])
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto str = const_cast<char16_t *>(literal);
    return { nullptr, str, N - 1 };
}
}

#define QStringLiteral(str) \
    (QString(QtPrivate::qMakeStringPrivate(QT_UNICODE_LITERAL(str)))) \
    /**/


QT_END_NAMESPACE

#endif // QSTRINGLITERAL_H
