// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

QUuid QUuid::fromString(QLatin1StringView string) noexcept
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

#endif // QT_CORE_REMOVED_SINCE(6, 3)

#if QT_CORE_REMOVED_SINCE(6, 4)

#include "qbytearray.h" // uses QT_CORE_INLINE_SINCE

#include "qcalendar.h"

QCalendar::QCalendar(QStringView name)
    : QCalendar(QAnyStringView{name}) {}

QCalendar::QCalendar(QLatin1StringView name)
    : QCalendar(QAnyStringView{name}) {}

#include "qcollator.h" // inline function compare(ptr, n, ptr, n) (for MSVC)

#include "qhashfunctions.h"

size_t qHash(const QByteArray &key, size_t seed) noexcept
{
    return qHashBits(key.constData(), size_t(key.size()), seed);
}

size_t qHash(const QByteArrayView &key, size_t seed) noexcept
{
    return qHashBits(key.constData(), size_t(key.size()), seed);
}

#include "qobject.h"

void QObject::setObjectName(const QString &name)
{
    setObjectName<void>(name);
}

#include "qlocale.h" // uses QT_CORE_INLINE_SINCE

#if QT_CONFIG(settings)

#include "qsettings.h"

void QSettings::beginGroup(const QString &prefix)
{
    return beginGroup(qToAnyStringViewIgnoringNull(prefix));
}

int QSettings::beginReadArray(const QString &prefix)
{
    return beginReadArray(qToAnyStringViewIgnoringNull(prefix));
}

void QSettings::beginWriteArray(const QString &prefix, int size)
{
    beginWriteArray(qToAnyStringViewIgnoringNull(prefix), size);
}

void QSettings::setValue(const QString &key, const QVariant &value)
{
    setValue(qToAnyStringViewIgnoringNull(key), value);
}

void QSettings::remove(const QString &key)
{
    remove(qToAnyStringViewIgnoringNull(key));
}

bool QSettings::contains(const QString &key) const
{
    return contains(qToAnyStringViewIgnoringNull(key));
}

QVariant QSettings::value(const QString &key, const QVariant &defaultValue) const
{
    return value(qToAnyStringViewIgnoringNull(key), defaultValue);
}

QVariant QSettings::value(const QString &key) const
{
    return value(qToAnyStringViewIgnoringNull(key));
}

#endif // QT_CONFIG(settings)

#include "qversionnumber.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

QVersionNumber QVersionNumber::fromString(const QString &string, int *suffixIndex)
{
    return fromString(qToAnyStringViewIgnoringNull(string), suffixIndex);
}

QVersionNumber QVersionNumber::fromString(QStringView string, int *suffixIndex)
{
    return fromString(QAnyStringView{string}, suffixIndex);
}

QVersionNumber QVersionNumber::fromString(QLatin1StringView string, int *suffixIndex)
{
    return fromString(QAnyStringView{string}, suffixIndex);
}

QT_WARNING_POP

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

#endif // QT_CORE_REMOVED_SINCE(6, 4)
