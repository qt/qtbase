/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QBYTEARRAYALGORITHMS_H
#define QBYTEARRAYALGORITHMS_H

#include <QtCore/qnamespace.h>

#if 0
#pragma qt_class(QByteArrayAlgorithms)
#endif

QT_BEGIN_NAMESPACE

class QByteArrayView;

namespace QtPrivate {

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
bool startsWith(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
bool endsWith(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype findByteArray(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype lastIndexOf(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION
qsizetype count(QByteArrayView haystack, QByteArrayView needle) noexcept;

[[nodiscard]] Q_CORE_EXPORT int compareMemory(QByteArrayView lhs, QByteArrayView rhs);

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QBYTEARRAYALGORITHMS_H
