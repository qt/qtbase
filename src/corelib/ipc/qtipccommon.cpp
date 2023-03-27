// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtipccommon.h"
#include "qtipccommon_p.h"

#include <qcryptographichash.h>
#include <qstandardpaths.h>
#include <qstringconverter.h>
#include <qurl.h>

#if defined(Q_OS_DARWIN)
#  include "private/qcore_mac_p.h"
#  if !defined(SHM_NAME_MAX)
     // Based on PSEMNAMLEN in XNU's posix_sem.c, which would
     // indicate the max length is 31, _excluding_ the zero
     // terminator. But in practice (possibly due to an off-
     // by-one bug in the kernel) the usable bytes are only 30.
#    define SHM_NAME_MAX 30
#  endif
#endif

#if QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QStringView staticTypeToString(QNativeIpcKey::Type type)
{
    switch (type) {
    case QNativeIpcKey::Type::SystemV:
        return u"systemv";
    case QNativeIpcKey::Type::PosixRealtime:
        return u"posix";
    case QNativeIpcKey::Type::Windows:
        return u"windows";
    }
    return {};
}

static QString typeToString(QNativeIpcKey::Type type)
{
    QStringView typeString = staticTypeToString(type);
    switch (type) {
    case QNativeIpcKey::Type::SystemV:
    case QNativeIpcKey::Type::PosixRealtime:
    case QNativeIpcKey::Type::Windows:
        return QString::fromRawData(typeString.constData(), typeString.size());
    }

    int value = int(type);
    if (value >= 1 && value <= 0xff) {
        // System V key with id different from 'Q'
        typeString = staticTypeToString(QNativeIpcKey::Type::SystemV);
        return typeString + QString::number(-value);   // negative so it prepends a dash
    }

    return QString();       // invalid!
}

static QNativeIpcKey::Type stringToType(QStringView typeString)
{
    if (typeString == staticTypeToString(QNativeIpcKey::Type::PosixRealtime))
        return QNativeIpcKey::Type::PosixRealtime;
    if (typeString == staticTypeToString(QNativeIpcKey::Type::Windows))
        return QNativeIpcKey::Type::Windows;

    auto fromNumber = [](QStringView number, int low, int high) {
        bool ok;
        int n = -number.toInt(&ok, 10);
        if (!ok || n < low || n > high)
            return QNativeIpcKey::Type{};
        return QNativeIpcKey::Type(n);
    };

    QStringView sysv = staticTypeToString(QNativeIpcKey::Type::SystemV);
    if (typeString.startsWith(sysv)) {
        if (typeString.size() == sysv.size())
            return QNativeIpcKey::Type::SystemV;
        return fromNumber(typeString.sliced(sysv.size()), 1, 0xff);
    }

    // invalid!
    return QNativeIpcKey::Type{};
}

/*!
    \internal

    Legacy: this exists for compatibility with QSharedMemory and
    QSystemSemaphore between 4.4 and 6.6.

    Generate a string from the key which can be any unicode string into
    the subset that the win/unix kernel allows.

    On Unix this will be a file name
*/
QString QtIpcCommon::legacyPlatformSafeKey(const QString &key, QtIpcCommon::IpcType ipcType,
                                           QNativeIpcKey::Type type)
{
    if (key.isEmpty())
        return QString();

    QByteArray hex = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();

    if (type == QNativeIpcKey::Type::PosixRealtime) {
#if defined(Q_OS_DARWIN)
        if (qt_apple_isSandboxed()) {
            // Sandboxed applications on Apple platforms require the shared memory name
            // to be in the form <application group identifier>/<custom identifier>.
            // Since we don't know which application group identifier the user wants
            // to apply, we instead document that requirement, and use the key directly.
            return key;
        }
        // The shared memory name limit on Apple platforms is very low (30 characters),
        // so we can't use the logic below of combining the prefix, key, and a hash,
        // to ensure a unique and valid name. Instead we use the first part of the
        // hash, which should still long enough to avoid collisions in practice.
        return u'/' + QLatin1StringView(hex).left(SHM_NAME_MAX - 1);
#endif
    }

    QString result;
    result.reserve(1 + 18 + key.size() + 40);
    switch (ipcType) {
    case IpcType::SharedMemory:
        result += "qipc_sharedmemory_"_L1;
        break;
    case IpcType::SystemSemaphore:
        result += "qipc_systemsem_"_L1;
        break;
    }

    for (QChar ch : key) {
        if ((ch >= u'a' && ch <= u'z') ||
           (ch >= u'A' && ch <= u'Z'))
           result += ch;
    }
    result.append(QLatin1StringView(hex));

    switch (type) {
    case QNativeIpcKey::Type::Windows:
        if (!isIpcSupported(ipcType, QNativeIpcKey::Type::Windows))
            return QString();
        return result;
    case QNativeIpcKey::Type::PosixRealtime:
        if (!isIpcSupported(ipcType, QNativeIpcKey::Type::PosixRealtime))
            return QString();
        return result.prepend(u'/');
    case QNativeIpcKey::Type::SystemV:
        break;
    }
    if (!isIpcSupported(ipcType, QNativeIpcKey::Type::SystemV))
        return QString();
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + u'/' + result;
}

QString QtIpcCommon::platformSafeKey(const QString &key, QtIpcCommon::IpcType ipcType,
                                     QNativeIpcKey::Type type)
{
    if (key.isEmpty())
        return key;

    switch (type) {
    case QNativeIpcKey::Type::PosixRealtime:
        if (!isIpcSupported(ipcType, QNativeIpcKey::Type::PosixRealtime))
            return QString();
#ifdef SHM_NAME_MAX
        // The shared memory name limit on Apple platforms is very low (30
        // characters), so we have to cut it down to avoid ENAMETOOLONG. We
        // hope that there won't be too many collisions...
        return u'/' + QStringView(key).left(SHM_NAME_MAX - 1);
#endif
        return u'/' + key;

    case QNativeIpcKey::Type::Windows:
        if (isIpcSupported(ipcType, QNativeIpcKey::Type::Windows)) {
            QStringView prefix;
            QStringView payload = key;
            // see https://learn.microsoft.com/en-us/windows/win32/termserv/kernel-object-namespaces
            for (QStringView candidate : { u"Local\\", u"Global\\" }) {
                if (!key.startsWith(candidate))
                    continue;
                prefix = candidate;
                payload = payload.sliced(prefix.size());
                break;
            }

            QStringView mid;
            switch (ipcType) {
            case IpcType::SharedMemory:     mid = u"shm_"; break;
            case IpcType::SystemSemaphore:  mid = u"sem_"; break;
            }

            QString result = prefix + mid + payload;
#ifdef MAX_PATH
            result.truncate(MAX_PATH);
#endif
            return result;
        }
        return QString();

    case QNativeIpcKey::Type::SystemV:
        break;
    }

    // System V
    if (!isIpcSupported(ipcType, QNativeIpcKey::Type::SystemV))
        return QString();
    if (key.startsWith(u'/'))
        return key;

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return baseDir + u'/' + key;
}

/*!
    \class QNativeIpcKey
    \inmodule QtCore
    \since 6.6
    \brief The QNativeIpcKey class holds a native key used by QSystemSemaphore and QSharedMemory.

    The \l QSharedMemory and \l QSystemSemaphore classes identify their
    resource using a system-wide identifier known as a "key". The low-level key
    value as well as the key type are encapsulated in Qt using the \l
    QNativeIpcKey class.

    Those two classes also provide the means to create native keys from a
    cross-platform identifier, using QSharedMemory::platformSafeKey() and
    QSystemSemaphore::platformSafeKey(). Applications should never share the
    input to those functions, as different versions of Qt may perform different
    transformations, resulting in different native keys. Instead, the
    application that created the IPC object should communicate the resulting
    native key using the methods described below.

    For details on the key types, platform-specific limitations, and
    interoperability with older or non-Qt applications, see the \l{Native IPC
    Keys} documentation. That includes important information for sandboxed
    applications on Apple platforms, including all apps obtained via the Apple
    App Store.

    \section1 Communicating keys to other processes
    \section2 Communicating keys to other Qt processes

    If the other process supports QNativeIpcKey, the best way of communicating
    is via the string representation obtained from toString() and parsing it
    using fromString(). This representation can be stored on a file whose name
    is well-known or passed on the command-line to a child process using
    QProcess::setArguments().

    If the other process does not support QNativeIpcKey, then the two processes
    can exchange the nativeKey() but the older code is likely unable to adjust
    its key type. The legacyDefaultTypeForOs() function returns the type that
    legacy code used, which may not match the \l{DefaultTypeForOs} constant.
    This is still true even if the old application is not using the same build
    as the new one (for example, it is a Qt 5 application), provided the
    options passed to the Qt configure script are the same.

    \section2 Communicating keys to non-Qt processes

    When communicating with non-Qt processes, the application must arrange to
    obtain the key type the other process is using. This is important
    particularly on Unix systems, where both \l PosixRealtime and \l SystemV
    are common.

    \section1 String representation of native keys

    The format of the string representation of a QNativeIpcKey is meant to be
    stable and therefore backwards and forwards compatible, provided the key
    type is supported by the Qt version in question. That is to say, an older
    Qt will fail to parse the string representation of a key type introduced
    after it was released. However, successfully parsing a string
    representation does not imply the Qt classes can successfully create an
    object of that type; applications should verify support using
    QSharedMemory::isKeyTypeSupported() and QSystemSemaphore::isKeyTypeSupported().

    The format of the string representation is formed by two components,
    separated by a colon (':'). The first component is the key type, described
    in the table below. The second component is a type-specific payload, using
    \l{QByteArray::fromPercentEncoding}{percent-encoding}. For all currently
    supported key types, the decoded form is identical to the contents of the
    nativeKey() field.

    \table
        \row    \li Key type            \li String representation
        \row    \li \l PosixRealtime    \li \c "posix"
        \row    \li \l SystemV          \li \c "systemv"
        \row    \li \l Windows          \li \c "windows"
        \row    \li Non-standard SystemV \li \c "systemv-" followed by a decimal number
    \endtable

    This format resembles a URI and allows parsing using URI/URL-parsing
    functions, such as \l QUrl. When parsed by such API, the key type will show
    up as the \l{QUrl::scheme()}{scheme}, and the payload will be the
    \l{QUrl::path()}{path}. Use of query or fragments is reserved.

    \sa QSharedMemory, QSystemSemaphore
*/

/*!
    \enum QNativeIpcKey::Type

    This enum describes the backend type for the IPC object. For details on the
    key types, see the \l{Native IPC Keys} documentation.

    \value SystemV          X/Open System Initiative (XSI) or System V (SVr4) API
    \value PosixRealtime    IEEE 1003.1b (POSIX.1b) API
    \value Windows          Win32 API

    \sa setType(), type()
*/

/*!
    \variable QNativeIpcKey::DefaultTypeForOs

    This constant expression variable holds the default native IPC type for the
    current OS. It will be Type::Windows for Windows systems and
    Type::PosixRealtime elsewhere. Note that this constant is different from
    what \l QSharedMemory and \l QSystemSemaphore defaulted to on the majority
    of Unix systems prior to Qt 6.6; see legacyDefaultTypeForOs() for more
    information.
*/

/*!
    \fn QNativeIpcKey::legacyDefaultTypeForOs() noexcept

    Returns the \l{Type} that corresponds to the native IPC key that
    \l{QSharedMemory} and \l{QSystemSemaphore} used to use prior to Qt 6.6.
    Applications and libraries that must retain compatibility with code using
    either class that was compiled with Qt prior to version 6.6 can use this
    function to determine what IPC type the other applications may be using.

    Note that this function relies on Qt having been built with identical
    configure-time options.
*/
#if defined(Q_OS_DARWIN)
QNativeIpcKey::Type QNativeIpcKey::defaultTypeForOs_internal() noexcept
{
    if (qt_apple_isSandboxed())
        return Type::PosixRealtime;
    return Type::SystemV;
}
#endif

/*!
    \fn QNativeIpcKey::QNativeIpcKey(Type type) noexcept
    \fn QNativeIpcKey::QNativeIpcKey(const QString &key, Type type)

    Constructs a QNativeIpcKey object holding native key \a key (or empty on
    the overload without the parameter) for type \a type.
*/

/*!
    \fn QNativeIpcKey::QNativeIpcKey(const QNativeIpcKey &other)
    \fn QNativeIpcKey::QNativeIpcKey(QNativeIpcKey &&other) noexcept
    \fn QNativeIpcKey &QNativeIpcKey::operator=(const QNativeIpcKey &other)
    \fn QNativeIpcKey &QNativeIpcKey::operator=(QNativeIpcKey &&other) noexcept

    Copies or moves the content of \a other.
*/
void QNativeIpcKey::copy_internal(const QNativeIpcKey &other)
{
    auto copy = new QNativeIpcKeyPrivate(*other.d_func());
    d = quintptr(copy) & 1;
}

void QNativeIpcKey::move_internal(QNativeIpcKey &&) noexcept
{
    // inline code already moved properly, nothing for us to do here
}

QNativeIpcKey &QNativeIpcKey::assign_internal(const QNativeIpcKey &other)
{
    QNativeIpcKeyPrivate *us = (d & 1) ? d_func() : nullptr;
    const QNativeIpcKeyPrivate *them = (other.d & 1) ? other.d_func() : nullptr;
    if (us && !them) {
        // don't need the extra info, reset to skinny object
        typeAndFlags = {};
        typeAndFlags.type = us->type;
        delete us;
    } else {
        // do need the extra info, so create if necessary
        if (us)
            *us = *them;
        else
            us = new QNativeIpcKeyPrivate(*them);
        d = quintptr(us) | 1;
    }
    return *this;
}

/*!
    \fn QNativeIpcKey::~QNativeIpcKey()

    Disposes of this QNativeIpcKey object.
*/
void QNativeIpcKey::destroy_internal() noexcept
{
    Q_D(QNativeIpcKey);
    delete d;
}

/*!
    \fn QNativeIpcKey::swap(QNativeIpcKey &other) noexcept
    \fn swap(QNativeIpcKey &lhs, QNativeIpcKey &rhs) noexcept

    Swaps the native IPC key and type \a other with this object, or \a lhs with
    \a rhs. This operation is very fast and never fails.
*/

/*!
    \fn QNativeIpcKey::isEmpty() const

    Returns true if the nativeKey() is empty.

    \sa nativeKey()
*/

/*!
    \fn QNativeIpcKey::isValid() const

    Returns true if this object contains a valid native IPC key type. Invalid
    types are usually the result of a failure to parse a string representation
    using fromString().

    This function performs no check on the whether the key string is actually
    supported or valid for the current operating system.

    \sa type(), fromString()
*/

/*!
    \fn QNativeIpcKey::type() const noexcept

    Returns the key type associated with this object.

    \sa nativeKey(), setType()
*/
QNativeIpcKey::Type QNativeIpcKey::type_internal() const noexcept
{
    Q_D(const QNativeIpcKey);
    return d->type;
}

/*!
    \fn QNativeIpcKey::setType(Type type)

    Sets the IPC type of this object to \a type.

    \sa type(), setNativeKey()
*/
void QNativeIpcKey::setType_internal(Type type)
{
    Q_D(QNativeIpcKey);
    d->type = type;
}

/*!
    \fn QNativeIpcKey::nativeKey() const noexcept

    Returns the native key string associated with this object.

    \sa setNativeKey(), type()
*/

/*!
    \fn QNativeIpcKey::setNativeKey(const QString &newKey)

    Sets the native key for this object to \a newKey.

    \sa nativeKey(), setType()
*/
void QNativeIpcKey::setNativeKey_internal(const QString &)
{
}

/*!
    \fn bool QNativeIpcKey::operator==(const QNativeIpcKey &lhs, const QNativeIpcKey &rhs) noexcept
    \fn bool QNativeIpcKey::operator!=(const QNativeIpcKey &lhs, const QNativeIpcKey &rhs) noexcept

    Returns true if the \a lhs and \a rhs objects hold the same (or different) contents.
*/
int QNativeIpcKey::compare_internal(const QNativeIpcKey &lhs, const QNativeIpcKey &rhs) noexcept
{
    return *lhs.d_func() == *rhs.d_func() ? 0 : 1;
}

/*!
    Returns the string representation of this object. String representations
    are useful to inform other processes of the key this process created and
    that they should attach to.

    This function returns a null string if the current object is
    \l{isValid()}{invalid}.

    \sa fromString()
*/
QString QNativeIpcKey::toString() const
{
    QString prefix = typeToString(type());
    if (prefix.isEmpty()) {
        Q_ASSERT(prefix.isNull());
        return prefix;
    }

    QString copy = nativeKey();
    copy.replace(u'%', "%25"_L1);
    if (copy.startsWith("//"_L1))
        copy.replace(0, 2, u"/%2F"_s);  // ensure it's parsed as a URL path

    QUrl u;
    u.setScheme(prefix);
    u.setPath(copy, QUrl::TolerantMode);
    return u.toString(QUrl::DecodeReserved);
}

/*!
    Parses the string form \a text and returns the corresponding QNativeIpcKey.
    String representations are useful to inform other processes of the key this
    process created and they should attach to.

    If the string could not be parsed, this function returns an
    \l{isValid()}{invalid} object.

    \sa toString(), isValid()
*/
QNativeIpcKey QNativeIpcKey::fromString(const QString &text)
{
    QUrl u(text, QUrl::TolerantMode);
    Type invalidType = {};
    Type type = stringToType(u.scheme());
    if (type == invalidType || !u.isValid() || !u.userInfo().isEmpty() || !u.host().isEmpty()
            || u.port() != -1)
        return QNativeIpcKey(invalidType);

    QNativeIpcKey result(QString(), type);
    if (result.type() != type)      // range check, just in case
        return QNativeIpcKey(invalidType);

    // decode the payload
    result.setNativeKey(u.path());
    return result;
}

QT_END_NAMESPACE

#include "moc_qtipccommon.cpp"

#endif // QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)
