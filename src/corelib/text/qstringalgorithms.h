/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#ifndef QSTRINGALGORITHMS_H
#define QSTRINGALGORITHMS_H

#include <QtCore/qnamespace.h>
#include <QtCore/qcontainerfwd.h>
#if 0
#pragma qt_class(QStringAlgorithms)
#endif

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
class QLatin1String;
class QStringView;
template <bool> class QBasicUtf8StringView;
class QAnyStringView;
class QChar;
class QRegularExpression;
class QRegularExpressionMatch;

namespace QtPrivate {

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype qustrlen(const char16_t *str) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION const char16_t *qustrchr(QStringView str, char16_t ch) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QLatin1String rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1String lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1String lhs, QLatin1String rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1String lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QBasicUtf8StringView<false> lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QBasicUtf8StringView<false> lhs, QLatin1String rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QBasicUtf8StringView<false> lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QStringView   lhs, QStringView   rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QStringView   lhs, QLatin1String rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QStringView   lhs, QBasicUtf8StringView<false> rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QLatin1String lhs, QStringView   rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QLatin1String lhs, QLatin1String rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QLatin1String lhs, QBasicUtf8StringView<false> rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QBasicUtf8StringView<false> lhs, QStringView   rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QBasicUtf8StringView<false> lhs, QLatin1String rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QBasicUtf8StringView<false> lhs, QBasicUtf8StringView<false> rhs) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QStringView   haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QStringView   haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QLatin1String haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QLatin1String haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QStringView   haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QStringView   haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QLatin1String haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QLatin1String haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QStringView haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QLatin1String haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QLatin1String haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QLatin1String haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QLatin1String haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QStringView   trimmed(QStringView   s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QLatin1String trimmed(QLatin1String s) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QStringView haystack, QChar needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

#if QT_CONFIG(regularexpression)
[[nodiscard]] Q_CORE_EXPORT qsizetype indexOf(QStringView haystack,
                                              const QRegularExpression &re,
                                              qsizetype from = 0,
                                              QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT qsizetype lastIndexOf(QStringView haystack,
                                                  const QRegularExpression &re,
                                                  qsizetype from = -1,
                                                  QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT bool contains(QStringView haystack,
                                          const QRegularExpression &re,
                                          QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT qsizetype count(QStringView haystack, const QRegularExpression &re);
#endif

[[nodiscard]] Q_CORE_EXPORT QString convertToQString(QAnyStringView s);

[[nodiscard]] Q_CORE_EXPORT QByteArray convertToLatin1(QStringView str);
[[nodiscard]] Q_CORE_EXPORT QByteArray convertToUtf8(QStringView str);
[[nodiscard]] Q_CORE_EXPORT QByteArray convertToLocal8Bit(QStringView str);
[[nodiscard]] Q_CORE_EXPORT QList<uint> convertToUcs4(QStringView str); // ### Qt 7 char32_t

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isRightToLeft(QStringView string) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isAscii(QLatin1String s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isAscii(QStringView   s) noexcept;
[[nodiscard]] constexpr inline                   bool isLatin1(QLatin1String s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isLatin1(QStringView   s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isValidUtf16(QStringView s) noexcept;

} // namespace QtPRivate

QT_END_NAMESPACE

#endif // QSTRINGALGORTIHMS_H
