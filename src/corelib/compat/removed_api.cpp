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

#if QT_CORE_REMOVED_SINCE(6, 5)

#include "qbasictimer.h" // inlined API

#include "qbuffer.h" // inline removed API

#include "qdir.h"

uint QDir::count() const
{
    return uint(count(QT6_CALL_NEW_OVERLOAD));
}

#if QT_POINTER_SIZE != 4
QString QDir::operator[](int i) const
{
    return operator[](qsizetype{i});
}
#endif

#include "qtenvironmentvariables.h"

bool qputenv(const char *varName, const QByteArray &value)
{
    return qputenv(varName, qToByteArrayViewIgnoringNull(value));
}

#include "qmetatype.h"

int QMetaType::idHelper() const
{
    Q_ASSERT(d_ptr);
    return registerHelper(d_ptr);
}

#if QT_CONFIG(sharedmemory)
#include "qsharedmemory.h"

void QSharedMemory::setNativeKey(const QString &key)
{
    setNativeKey(key, QNativeIpcKey::legacyDefaultTypeForOs());
}
#endif

#include "qvariant.h"

// these implementations aren't as efficient as they used to be prior to
// replacement, but there's no way to call the ambiguous overload
QVariant::QVariant(const QUuid &uuid) : QVariant(QVariant::fromValue(uuid)) {}
#ifndef QT_NO_GEOM_VARIANT
#include "qline.h"
#include "qpoint.h"
#include "qrect.h"
#include "qsize.h"
QVariant::QVariant(const QPoint &pt) : QVariant(QVariant::fromValue(pt)) {}
QVariant::QVariant(const QPointF &pt) : QVariant(QVariant::fromValue(pt)) {}
QVariant::QVariant(const QRect &r) : QVariant(QVariant::fromValue(r)) {}
QVariant::QVariant(const QRectF &r) : QVariant(QVariant::fromValue(r)) {}
QVariant::QVariant(const QLine &l) : QVariant(QVariant::fromValue(l)) {}
QVariant::QVariant(const QLineF &l) : QVariant(QVariant::fromValue(l)) {}
QVariant::QVariant(const QSize &s) : QVariant(QVariant::fromValue(s)) {}
QVariant::QVariant(const QSizeF &s) : QVariant(QVariant::fromValue(s)) {}
#endif

#if QT_CONFIG(xmlstreamreader)

#include "qxmlstream.h"

QXmlStreamReader::QXmlStreamReader(const QByteArray &data)
    : QXmlStreamReader(data, PrivateConstructorTag{})
{
}

QXmlStreamReader::QXmlStreamReader(const QString &data)
    : QXmlStreamReader(qToAnyStringViewIgnoringNull(data))
{
}

QXmlStreamReader::QXmlStreamReader(const char *data)
    : QXmlStreamReader(QAnyStringView(data))
{
}

void QXmlStreamReader::addData(const QByteArray &data)
{
    addData<>(data);
}
void QXmlStreamReader::addData(const QString &data)
{
    addData(qToAnyStringViewIgnoringNull(data));
}

void QXmlStreamReader::addData(const char *data)
{
    addData(QAnyStringView(data));
}

#endif // QT_CONFIG(xmlstreamreader)

#if QT_CONFIG(xmlstreamwriter)

#include "qxmlstream.h"

void QXmlStreamWriter::writeAttribute(const QString &qualifiedName, const QString &value)
{
    writeAttribute(qToAnyStringViewIgnoringNull(qualifiedName),
                   qToAnyStringViewIgnoringNull(value));
}

void QXmlStreamWriter::writeAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    writeAttribute(qToAnyStringViewIgnoringNull(namespaceUri),
                   qToAnyStringViewIgnoringNull(name),
                   qToAnyStringViewIgnoringNull(value));
}

void QXmlStreamWriter::writeCDATA(const QString &text)
{
    writeCDATA(qToAnyStringViewIgnoringNull(text));
}

void QXmlStreamWriter::writeCharacters(const QString &text)
{
    writeCharacters(qToAnyStringViewIgnoringNull(text));
}

void QXmlStreamWriter::writeComment(const QString &text)
{
    writeComment(qToAnyStringViewIgnoringNull(text));
}

void QXmlStreamWriter::writeDTD(const QString &dtd)
{
    writeDTD(qToAnyStringViewIgnoringNull(dtd));
}

void QXmlStreamWriter::writeEmptyElement(const QString &qualifiedName)
{
    writeEmptyElement(qToAnyStringViewIgnoringNull(qualifiedName));
}

void QXmlStreamWriter::writeEmptyElement(const QString &namespaceUri, const QString &name)
{
    writeEmptyElement(qToAnyStringViewIgnoringNull(namespaceUri),
                      qToAnyStringViewIgnoringNull(name));
}

void QXmlStreamWriter::writeTextElement(const QString &qualifiedName, const QString &text)
{
    writeTextElement(qToAnyStringViewIgnoringNull(qualifiedName),
                     qToAnyStringViewIgnoringNull(text));
}

void QXmlStreamWriter::writeTextElement(const QString &namespaceUri, const QString &name, const QString &text)
{
    writeTextElement(qToAnyStringViewIgnoringNull(namespaceUri),
                     qToAnyStringViewIgnoringNull(name),
                     qToAnyStringViewIgnoringNull(text));
}

void QXmlStreamWriter::writeEntityReference(const QString &name)
{
    writeEntityReference(qToAnyStringViewIgnoringNull(name));
}

void QXmlStreamWriter::writeNamespace(const QString &namespaceUri, const QString &prefix)
{
    writeNamespace(qToAnyStringViewIgnoringNull(namespaceUri),
                   qToAnyStringViewIgnoringNull(prefix));
}

void QXmlStreamWriter::writeDefaultNamespace(const QString &namespaceUri)
{
    writeDefaultNamespace(qToAnyStringViewIgnoringNull(namespaceUri));
}

void QXmlStreamWriter::writeProcessingInstruction(const QString &target, const QString &data)
{
    writeProcessingInstruction(qToAnyStringViewIgnoringNull(target),
                               qToAnyStringViewIgnoringNull(data));
}

void QXmlStreamWriter::writeStartDocument(const QString &version)
{
    writeStartDocument(qToAnyStringViewIgnoringNull(version));
}

void QXmlStreamWriter::writeStartDocument(const QString &version, bool standalone)
{
    writeStartDocument(qToAnyStringViewIgnoringNull(version),
                       standalone);
}

void QXmlStreamWriter::writeStartElement(const QString &qualifiedName)
{
    writeStartElement(qToAnyStringViewIgnoringNull(qualifiedName));
}

void QXmlStreamWriter::writeStartElement(const QString &namespaceUri, const QString &name)
{
    writeStartElement(qToAnyStringViewIgnoringNull(namespaceUri),
                      qToAnyStringViewIgnoringNull(name));
}

#endif // QT_CONFIG(xmlstreamwriter)

// inlined API
#include "qfloat16.h"
#include "qstring.h"

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

#endif // QT_CORE_REMOVED_SINCE(6, 5)

#if QT_CORE_REMOVED_SINCE(6, 6)

#include "qmessageauthenticationcode.h"

QMessageAuthenticationCode::QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                                       const QByteArray &key)
    : QMessageAuthenticationCode(method, qToByteArrayViewIgnoringNull(key)) {}

void QMessageAuthenticationCode::setKey(const QByteArray &key)
{
    setKey(qToByteArrayViewIgnoringNull(key));
}

void QMessageAuthenticationCode::addData(const QByteArray &data)
{
    addData(qToByteArrayViewIgnoringNull(data));
}

QByteArray QMessageAuthenticationCode::hash(const QByteArray &msg, const QByteArray &key,
                                            QCryptographicHash::Algorithm method)
{
    return hash(qToByteArrayViewIgnoringNull(msg),
                qToByteArrayViewIgnoringNull(key), method);
}

#include "qobject.h" // inlined API

#include "qrunnable.h"

QRunnable *QRunnable::create(std::function<void()> functionToRun)
{
    return QRunnable::create<std::function<void()>>(std::move(functionToRun));
}

#include "qstring.h"

qsizetype QString::toUcs4_helper(const ushort *uc, qsizetype length, uint *out)
{
    return toUcs4_helper(reinterpret_cast<const char16_t *>(uc), length,
                         reinterpret_cast<char32_t *>(out));
}

#if QT_CONFIG(thread)
#include "qreadwritelock.h"

bool QReadWriteLock::tryLockForRead()
{
    return tryLockForRead(0);
}

bool QReadWriteLock::tryLockForWrite()
{
    return tryLockForWrite(0);
}

#include "qthreadpool.h"
#include "private/qthreadpool_p.h"

void QThreadPool::start(std::function<void()> functionToRun, int priority)
{
    if (!functionToRun)
        return;
    start(QRunnable::create(std::move(functionToRun)), priority);
}

bool QThreadPool::tryStart(std::function<void()> functionToRun)
{
    if (!functionToRun)
        return false;

    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    if (!d->allThreads.isEmpty() && d->areAllThreadsActive())
        return false;

    QRunnable *runnable = QRunnable::create(std::move(functionToRun));
    if (d->tryStart(runnable))
        return true;
    delete runnable;
    return false;
}

void QThreadPool::startOnReservedThread(std::function<void()> functionToRun)
{
    if (!functionToRun)
        return releaseThread();

    startOnReservedThread(QRunnable::create(std::move(functionToRun)));
}

#endif // QT_CONFIG(thread)

#include "qxmlstream.h"

QStringView QXmlStreamAttributes::value(const QString &namespaceUri, const QString &name) const
{
    return value(qToAnyStringViewIgnoringNull(namespaceUri), qToAnyStringViewIgnoringNull(name));
}

QStringView QXmlStreamAttributes::value(const QString &namespaceUri, QLatin1StringView name) const
{
    return value(qToAnyStringViewIgnoringNull(namespaceUri), QAnyStringView(name));
}

QStringView QXmlStreamAttributes::value(QLatin1StringView namespaceUri, QLatin1StringView name) const
{
    return value(QAnyStringView(namespaceUri), QAnyStringView(name));
}

QStringView QXmlStreamAttributes::value(const QString &qualifiedName) const
{
    return value(qToAnyStringViewIgnoringNull(qualifiedName));
}

QStringView QXmlStreamAttributes::value(QLatin1StringView qualifiedName) const
{
    return value(QAnyStringView(qualifiedName));
}

// inlined API
#if QT_CONFIG(thread)
#include "qmutex.h"
#include "qreadwritelock.h"
#include "qsemaphore.h"
#endif

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

#endif // QT_CORE_REMOVED_SINCE(6, 6)
