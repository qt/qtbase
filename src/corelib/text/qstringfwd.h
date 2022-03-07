/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtCore/qglobal.h>

#ifndef QSTRINGFWD_H
#define QSTRINGFWD_H

QT_BEGIN_NAMESPACE

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
# define QT_BEGIN_HAS_CHAR8_T_NAMESPACE inline namespace q_has_char8_t {
# define QT_BEGIN_NO_CHAR8_T_NAMESPACE namespace q_no_char8_t {
#else
# define QT_BEGIN_HAS_CHAR8_T_NAMESPACE namespace q_has_char8_t {
# define QT_BEGIN_NO_CHAR8_T_NAMESPACE inline namespace q_no_char8_t {
#endif
#define QT_END_HAS_CHAR8_T_NAMESPACE }
#define QT_END_NO_CHAR8_T_NAMESPACE }

// declare namespaces:
QT_BEGIN_HAS_CHAR8_T_NAMESPACE
QT_END_HAS_CHAR8_T_NAMESPACE
QT_BEGIN_NO_CHAR8_T_NAMESPACE
QT_END_NO_CHAR8_T_NAMESPACE

class QByteArray;
class QByteArrayView;
class QLatin1String;
class QStringView;
template <bool> class QBasicUtf8StringView;
class QAnyStringView;
class QChar;
class QRegularExpression;
class QRegularExpressionMatch;

#ifndef Q_CLANG_QDOC
// ### Qt 7: remove the non-char8_t version of QUtf8StringView
QT_BEGIN_NO_CHAR8_T_NAMESPACE
using QUtf8StringView = QBasicUtf8StringView<false>;
QT_END_NO_CHAR8_T_NAMESPACE

QT_BEGIN_HAS_CHAR8_T_NAMESPACE
using QUtf8StringView = QBasicUtf8StringView<true>;
QT_END_HAS_CHAR8_T_NAMESPACE
#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // QSTRINGFWD_H
