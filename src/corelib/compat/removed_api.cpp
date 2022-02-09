/****************************************************************************
**
** Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
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

#define QT_CORE_BUILD_REMOVED_API

#include "qglobal.h"

QT_USE_NAMESPACE

#if QT_CORE_REMOVED_SINCE(6, 1)

#include "qmetatype.h"

// keep in sync with version in header
int QMetaType::id() const
{
    if (d_ptr) {
        if (int id = d_ptr->typeId.loadRelaxed())
            return id;
        return idHelper();
    }
    return 0;
}

#endif // QT_CORE_REMOVED_SINCE(6, 1)

#if QT_CORE_REMOVED_SINCE(6, 2)

#include "qbindingstorage.h"

void QBindingStorage::maybeUpdateBindingAndRegister_helper(const QUntypedPropertyData *data) const
{
    registerDependency_helper(data);
}

#endif // QT_CORE_REMOVED_SINCE(6, 2)

#if QT_CORE_REMOVED_SINCE(6, 3)

#include "qbytearraymatcher.h"

# if QT_POINTER_SIZE != 4

int QStaticByteArrayMatcherBase::indexOfIn(const char *h, uint hl, const char *n, int nl, int from) const noexcept
{
    qsizetype r = indexOfIn(h, size_t(hl), n, qsizetype(nl), qsizetype(from));
    Q_ASSERT(r == int(r));
    return r;
}

# endif // QT_POINTER_SIZE != 4

qsizetype QByteArrayMatcher::indexIn(const QByteArray &ba, qsizetype from) const
{
    return indexIn(QByteArrayView{ba}, from); // ba.isNull() may be significant, so don't ignore it!
}

#include "tools/qcryptographichash.h"

void QCryptographicHash::addData(const QByteArray &data)
{
    addData(QByteArrayView{data});
}

QByteArray QCryptographicHash::hash(const QByteArray &data, Algorithm method)
{
    return hash(QByteArrayView{data}, method);
}

#include "qdatastream.h"

# ifndef QT_NO_DATASTREAM
# include "qfloat16.h"

QDataStream &QDataStream::operator>>(qfloat16 &f)
{
    return *this >> reinterpret_cast<qint16&>(f);
}

QDataStream &QDataStream::operator<<(qfloat16 f)
{
    return *this << reinterpret_cast<qint16&>(f);
}

# endif

#include "quuid.h"

QUuid::QUuid(const QString &text)
    : QUuid{qToAnyStringViewIgnoringNull(text)}
{
}

QUuid::QUuid(const char *text)
    : QUuid{QAnyStringView(text)}
{
}

QUuid::QUuid(const QByteArray &text)
    : QUuid{qToAnyStringViewIgnoringNull(text)}
{
}

QUuid QUuid::fromString(QStringView string) noexcept
{
    return fromString(QAnyStringView{string});
}

QUuid QUuid::fromString(QLatin1String string) noexcept
{
    return fromString(QAnyStringView{string});
}

QUuid QUuid::fromRfc4122(const QByteArray &bytes)
{
    return fromRfc4122(qToByteArrayViewIgnoringNull(bytes));
}

#include "qbytearraylist.h"

# if QT_POINTER_SIZE != 4
QByteArray QtPrivate::QByteArrayList_join(const QByteArrayList *that, const char *sep, int seplen)
{
    return QByteArrayList_join(that, sep, qsizetype(seplen));
}
# endif

#include "qlocale.h"

QString QLocale::languageToCode(Language language)
{
    return languageToCode(language, QLocale::AnyLanguageCode);
}

QLocale::Language QLocale::codeToLanguage(QStringView languageCode) noexcept
{
    return codeToLanguage(languageCode, QLocale::AnyLanguageCode);
}

#include "qoperatingsystemversion.h"

int QOperatingSystemVersion::compare(const QOperatingSystemVersion &v1,
                                     const QOperatingSystemVersion &v2)
{
    return QOperatingSystemVersionBase::compare(v1, v2);
}

#include "qurl.h"

QString QUrl::fromAce(const QByteArray &domain)
{
    return fromAce(domain, {});
}

QByteArray QUrl::toAce(const QString &domain)
{
    return toAce(domain, {});
}

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h

#endif // QT_CORE_REMOVED_SINCE(6, 3)
