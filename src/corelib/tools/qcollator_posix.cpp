/****************************************************************************
**
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "qcollator_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <cstring>
#include <cwchar>

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
    if (locale != QLocale())
        qWarning("Only default locale supported with the posix collation implementation");
    if (caseSensitivity != Qt::CaseSensitive)
        qWarning("Case insensitive sorting unsupported in the posix collation implementation");
    if (numericMode)
        qWarning("Numeric mode unsupported in the posix collation implementation");
    if (ignorePunctuation)
        qWarning("Ignoring punctuation unsupported in the posix collation implementation");
    dirty = false;
}

void QCollatorPrivate::cleanup()
{
}

static void stringToWCharArray(QVarLengthArray<wchar_t> &ret, const QString &string)
{
    ret.resize(string.length());
    int len = string.toWCharArray(ret.data());
    ret.resize(len+1);
    ret[len] = 0;
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    QVarLengthArray<wchar_t> array1, array2;
    stringToWCharArray(array1, QString(s1, len1));
    stringToWCharArray(array2, QString(s2, len2));
    return std::wcscoll(array1.constData(), array2.constData());
}

int QCollator::compare(const QString &s1, const QString &s2) const
{
    QVarLengthArray<wchar_t> array1, array2;
    stringToWCharArray(array1, s1);
    stringToWCharArray(array2, s2);
    return std::wcscoll(array1.constData(), array2.constData());
}

int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
{
    if (d->dirty)
        d->init();

    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (d->dirty)
        d->init();

    QVarLengthArray<wchar_t> original;
    stringToWCharArray(original, string);
    QVector<wchar_t> result(string.size());
    size_t size = std::wcsxfrm(result.data(), original.constData(), string.size());
    if (size > uint(result.size())) {
        result.resize(size+1);
        size = std::wcsxfrm(result.data(), original.constData(), string.size());
    }
    result.resize(size+1);
    result[size] = 0;
    return QCollatorSortKey(new QCollatorSortKeyPrivate(result));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return std::wcscmp(d->m_key.constData(), otherKey.d->m_key.constData());
}

QT_END_NAMESPACE
