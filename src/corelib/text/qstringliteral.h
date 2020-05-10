/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2020 Intel Corporation.
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

typedef char16_t qunicodechar;

Q_STATIC_ASSERT_X(sizeof(qunicodechar) == 2,
        "qunicodechar must typedef an integral type of size 2");

#define QT_UNICODE_LITERAL(str) u"" str
#define QStringLiteral(str) \
    ([]() noexcept -> QString { \
        enum { Size = sizeof(QT_UNICODE_LITERAL(str))/2 - 1 }; \
        static const QArrayData qstring_literal = { \
            Q_BASIC_ATOMIC_INITIALIZER(-1), QArrayData::StaticDataFlags, 0 \
        }; \
        QStringPrivate holder = {  \
            static_cast<QTypedArrayData<char16_t> *>(const_cast<QArrayData *>(&qstring_literal)), \
            const_cast<qunicodechar *>(QT_UNICODE_LITERAL(str)), \
            Size \
        }; \
        return QString(holder); \
    }()) \
    /**/

using QStringPrivate = QArrayDataPointer<char16_t>;

QT_END_NAMESPACE

#endif // QSTRINGLITERAL_H
