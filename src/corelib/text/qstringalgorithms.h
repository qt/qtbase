/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#ifndef QSTRINGALGORITHMS_H
#define QSTRINGALGORITHMS_H

#include <QtCore/qnamespace.h>

#if 0
#pragma qt_class(QStringAlgorithms)
#endif

QT_BEGIN_NAMESPACE

class QByteArray;
class QLatin1String;
class QStringView;
class QChar;
template <typename T> class QVector;

namespace QtPrivate {

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype qustrlen(const ushort *str) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION const ushort *qustrchr(QStringView str, ushort ch) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QLatin1String rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1String lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1String lhs, QLatin1String rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;


Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QStringView   haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QStringView   haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QLatin1String haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QLatin1String haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QStringView   haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QStringView   haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QLatin1String haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QLatin1String haystack, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QStringView haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QLatin1String haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QLatin1String haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QLatin1String haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QLatin1String haystack, qsizetype from, QLatin1String needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QStringView   trimmed(QStringView   s) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QLatin1String trimmed(QLatin1String s) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT QByteArray convertToLatin1(QStringView str);
Q_REQUIRED_RESULT Q_CORE_EXPORT QByteArray convertToUtf8(QStringView str);
Q_REQUIRED_RESULT Q_CORE_EXPORT QByteArray convertToLocal8Bit(QStringView str);
Q_REQUIRED_RESULT Q_CORE_EXPORT QVector<uint> convertToUcs4(QStringView str);

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isRightToLeft(QStringView string) noexcept;

Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isAscii(QLatin1String s) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isAscii(QStringView   s) noexcept;
Q_REQUIRED_RESULT Q_DECL_CONSTEXPR inline            bool isLatin1(QLatin1String s) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isLatin1(QStringView   s) noexcept;
Q_REQUIRED_RESULT Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isValidUtf16(QStringView s) noexcept;

} // namespace QtPRivate

QT_END_NAMESPACE

#endif // QSTRINGALGORTIHMS_H
