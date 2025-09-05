// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtenvironmentvariables.h"
#include "qtenvironmentvariables_p.h"

#include <qplatformdefs.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>

#include <QtCore/private/qlocale_p.h>
#include <QtCore/private/qlocking_p.h>

QT_BEGIN_NAMESPACE

// In the C runtime on all platforms access to the environment is not thread-safe. We
// add thread-safety for the Qt wrappers.
Q_CONSTINIT static QBasicMutex environmentMutex;

/*!
    \headerfile <QtEnvironmentVariables>
    \inmodule QtCore
    \ingroup funclists
    \brief Helper functions for working with environment variables.
*/

/*!
    \relates <QtEnvironmentVariables>
    \threadsafe

    Returns the value of the environment variable with name \a varName as a
    QByteArray. If no variable by that name is found in the environment, this
    function returns a default-constructed QByteArray.

    The Qt environment manipulation functions are thread-safe, but this
    requires that the C library equivalent functions like getenv and putenv are
    not directly called.

    To convert the data to a QString use QString::fromLocal8Bit().

    \note on desktop Windows, qgetenv() may produce data loss if the
    original string contains Unicode characters not representable in the
    ANSI encoding. Use qEnvironmentVariable() instead.
    On Unix systems, this function is lossless.

    \sa qputenv(), qEnvironmentVariable(), qEnvironmentVariableIsSet(),
    qEnvironmentVariableIsEmpty(), qEnvironmentVariableIntegerValue()
*/
QByteArray qgetenv(const char *varName)
{
    const auto locker = qt_scoped_lock(environmentMutex);
    return QByteArray(::getenv(varName));
}

/*!
    \fn QString qEnvironmentVariable(const char *varName, const QString &defaultValue)
    \fn QString qEnvironmentVariable(const char *varName)

    \relates <QtEnvironmentVariables>
    \since 5.10

    These functions return the value of the environment variable, \a varName, as a
    QString. If no variable \a varName is found in the environment and \a defaultValue
    is provided, \a defaultValue is returned. Otherwise QString() is returned.

    The Qt environment manipulation functions are thread-safe, but this
    requires that the C library equivalent functions like getenv and putenv are
    not directly called.

    The following table describes how to choose between qgetenv() and
    qEnvironmentVariable():
    \table
      \header \li Condition         \li Recommendation
      \row
        \li Variable contains file paths or user text
        \li qEnvironmentVariable()
      \row
        \li Windows-specific code
        \li qEnvironmentVariable()
      \row
        \li Unix-specific code, destination variable is not QString and/or is
            used to interface with non-Qt APIs
        \li qgetenv()
      \row
        \li Destination variable is a QString
        \li qEnvironmentVariable()
      \row
        \li Destination variable is a QByteArray or std::string
        \li qgetenv()
    \endtable

    \note on Unix systems, this function may produce data loss if the original
    string contains arbitrary binary data that cannot be decoded by the locale
    codec. Use qgetenv() instead for that case. On Windows, this function is
    lossless.

    \note the variable name \a varName must contain only US-ASCII characters.

    \sa qputenv(), qgetenv(), qEnvironmentVariableIsSet(), qEnvironmentVariableIsEmpty(),
        qEnvironmentVariableIntegerValue()
*/
QString qEnvironmentVariable(const char *varName, const QString &defaultValue)
{
#if defined(Q_OS_WIN)
    QVarLengthArray<wchar_t, 32> wname(qsizetype(strlen(varName)) + 1);
    for (qsizetype i = 0; i < wname.size(); ++i) // wname.size() is correct: will copy terminating null
        wname[i] = uchar(varName[i]);
    size_t requiredSize = 0;
    auto locker = qt_unique_lock(environmentMutex);
    _wgetenv_s(&requiredSize, 0, 0, wname.data());
    if (requiredSize == 0)
        return defaultValue;
    QString buffer(qsizetype(requiredSize), Qt::Uninitialized);
    _wgetenv_s(&requiredSize, reinterpret_cast<wchar_t *>(buffer.data()), requiredSize,
               wname.data());
    locker.unlock();
    // requiredSize includes the terminating null, which we don't want.
    Q_ASSERT(buffer.endsWith(QChar(u'\0')));
    buffer.chop(1);
    return buffer;
#else
    const auto locker = qt_scoped_lock(environmentMutex);
    const char *value = ::getenv(varName);
    if (!value)
        return defaultValue;
// duplicated in qfile.h (QFile::decodeName)
#if defined(Q_OS_DARWIN)
    return QString::fromUtf8(value).normalized(QString::NormalizationForm_C);
#else // other Unix
    return QString::fromLocal8Bit(value);
#endif
#endif
}

QString qEnvironmentVariable(const char *varName)
{
    return qEnvironmentVariable(varName, QString());
}

/*!
    \relates <QtEnvironmentVariables>
    \since 5.1

    Returns whether the environment variable \a varName is empty.

    Equivalent to
    \snippet code/src_corelib_global_qglobal.cpp is-empty
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariable(), qEnvironmentVariableIsSet()
*/
bool qEnvironmentVariableIsEmpty(const char *varName) noexcept
{
    const auto locker = qt_scoped_lock(environmentMutex);
    const char * const value = ::getenv(varName);
    return !value || !*value;
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized") // older GCC don't like libstdc++'s std::optional
/*!
    \relates <QtEnvironmentVariables>
    \since 5.5

    Returns the numerical value of the environment variable \a varName.
    If \a ok is not null, sets \c{*ok} to \c true or \c false depending
    on the success of the conversion.

    Equivalent to
    \snippet code/src_corelib_global_qglobal.cpp to-int
    except that it's much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariable(), qEnvironmentVariableIsSet()
*/
int qEnvironmentVariableIntValue(const char *varName, bool *ok) noexcept
{
    std::optional<qint64> value = qEnvironmentVariableIntegerValue(varName);
    if (value && *value != int(*value))
        value = std::nullopt;
    if (ok)
        *ok = bool(value);
    return value.value_or(0);
}
QT_WARNING_POP

/*!
    \relates <QtEnvironmentVariables>
    \since 6.10

    Returns the numerical value of the environment variable \a varName. If the
    variable is not set or could not be parsed as an integer, it returns
    \c{std::nullopt}.

    Similar to
    \snippet code/src_corelib_global_qglobal.cpp to-int
    except that it's much faster, and can't throw exceptions.

    If a value of zero is semantically the same as an empty or unset variable,
    applications can use
    \snippet code/src_corelib_global_qglobal.cpp int-value_or
    Do note in this case that failures to parse a value will also produce a
    zero.

    But if a value of zero can be used to disable some functionality,
    applications can compare the returned \c{std::optional} to zero, which will
    only be true if the variable was set and contained a number that parsed as
    zero, as in:
    \snippet code/src_corelib_global_qglobal.cpp int-eq0

    \sa qgetenv(), qEnvironmentVariable(), qEnvironmentVariableIsSet()
*/
std::optional<qint64> qEnvironmentVariableIntegerValue(const char *varName) noexcept
{
    const auto locker = qt_scoped_lock(environmentMutex);
    const char * const buffer = ::getenv(varName);
    if (!buffer)
        return std::nullopt;
    auto r = QLocaleData::bytearrayToLongLong(buffer, 0);
    if (!r.ok())
        return std::nullopt;
    return r.result;
}

/*!
    \relates <QtEnvironmentVariables>
    \since 5.1

    Returns whether the environment variable \a varName is set.

    Equivalent to
    \snippet code/src_corelib_global_qglobal.cpp is-null
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariable(), qEnvironmentVariableIsEmpty(),
        qEnvironmentVariableIntegerValue()
*/
bool qEnvironmentVariableIsSet(const char *varName) noexcept
{
    const auto locker = qt_scoped_lock(environmentMutex);
    return ::getenv(varName) != nullptr;
}

/*!
    \fn bool qputenv(const char *varName, QByteArrayView value)
    \relates <QtEnvironmentVariables>

    This function sets the \a value of the environment variable named
    \a varName. It will create the variable if it does not exist. It
    returns 0 if the variable could not be set.

    Calling qputenv with an empty value removes the environment variable on
    Windows, and makes it set (but empty) on Unix. Prefer using qunsetenv()
    for fully portable behavior.

    \note qputenv() was introduced because putenv() from the standard
    C library was deprecated in VC2005 (and later versions). qputenv()
    uses the replacement function in VC, and calls the standard C
    library's implementation on all other platforms.

    \note In Qt versions prior to 6.5, the \a value argument was QByteArray,
    not QByteArrayView.

    \sa qgetenv(), qEnvironmentVariable()
*/
bool qputenv(const char *varName, QByteArrayView raw)
{
    auto protect = [](const char *str) { return str ? str : ""; };

    std::string value{protect(raw.data()), size_t(raw.size())}; // NUL-terminates w/SSO

#if defined(Q_CC_MSVC)
    const auto locker = qt_scoped_lock(environmentMutex);
    return _putenv_s(varName, value.data()) == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(Q_OS_HAIKU)
    // POSIX.1-2001 has setenv
    const auto locker = qt_scoped_lock(environmentMutex);
    return setenv(varName, value.data(), true) == 0;
#else
    std::string buffer;
    buffer += protect(varName);
    buffer += '=';
    buffer += value;
    char *envVar = qstrdup(buffer.data());
    int result = [&] {
        const auto locker = qt_scoped_lock(environmentMutex);
        return putenv(envVar);
    }();
    if (result != 0) // error. we have to delete the string.
        delete[] envVar;
    return result == 0;
#endif
}

/*!
    \relates <QtEnvironmentVariables>

    This function deletes the variable \a varName from the environment.

    Returns \c true on success.

    \since 5.1

    \sa qputenv(), qgetenv(), qEnvironmentVariable()
*/
bool qunsetenv(const char *varName)
{
#if defined(Q_CC_MSVC)
    const auto locker = qt_scoped_lock(environmentMutex);
    return _putenv_s(varName, "") == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(Q_OS_BSD4) || defined(Q_OS_HAIKU)
    // POSIX.1-2001, BSD and Haiku have unsetenv
    const auto locker = qt_scoped_lock(environmentMutex);
    return unsetenv(varName) == 0;
#elif defined(Q_CC_MINGW)
    // On mingw, putenv("var=") removes "var" from the environment
    QByteArray buffer(varName);
    buffer += '=';
    const auto locker = qt_scoped_lock(environmentMutex);
    return putenv(buffer.constData()) == 0;
#else
    // Fallback to putenv("var=") which will insert an empty var into the
    // environment and leak it
    QByteArray buffer(varName);
    buffer += '=';
    char *envVar = qstrdup(buffer.constData());
    const auto locker = qt_scoped_lock(environmentMutex);
    return putenv(envVar) == 0;
#endif
}

/* Various time-related APIs that need to consult system settings also need
   protection with the same lock as the environment, since those system settings
   include part of the environment (principally TZ).

   First, tzset(), which POSIX explicitly says accesses the environment.
*/
void qTzSet()
{
    const auto locker = qt_scoped_lock(environmentMutex);
#if defined(Q_OS_WIN)
    _tzset();
#else
    tzset();
#endif // Q_OS_WIN
}

/* Wrap mktime(), which is specified to behave as if it called tzset(), hence
   shares its implicit environment-dependence.
*/
time_t qMkTime(struct tm *when)
{
    const auto locker = qt_scoped_lock(environmentMutex);
#if defined(Q_OS_WIN)
    // QTBUG-83881 MS's mktime() seems to need _tzset() called first.
    _tzset();
#endif
    return mktime(when);
}

/* For localtime(), POSIX mandates that it behave as if it called tzset().
   For the alternatives to it, we need (if only for compatibility) to do the
   same explicitly, which should ensure a re-parse of timezone info.
*/
bool qLocalTime(time_t utc, struct tm *local)
{
    const auto locker = qt_scoped_lock(environmentMutex);
#if defined(Q_OS_WIN)
    // The doc of localtime_s() says that it corrects for the same things
    // _tzset() sets the globals for, but QTBUG-109974 reveals a need for an
    // explicit call, all the same.
    _tzset();
    return !localtime_s(local, &utc);
#elif QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    // Use the reentrant version of localtime() where available, as it is
    // thread-safe and doesn't use a shared static data area.
    // As localtime_r() is not specified to work as if it called tzset(),
    // make an explicit call.
    tzset();
    if (tm *res = localtime_r(&utc, local)) {
        Q_ASSERT(res == local);
        Q_UNUSED(res);
        return true;
    }
    return false;
#else
    // POSIX mandates that localtime() behaves as if it called tzset().
    // Returns shared static data which may be overwritten at any time (albeit
    // our lock probably keeps it safe). So copy the result promptly:
    if (tm *res = localtime(&utc)) {
        *local = *res;
        return true;
    }
    return false;
#endif
}

/* Access to the tzname[] global in one thread is UB if any other is calling
   tzset() or anything that behaves as if it called tzset(). So also lock this
   access to prevent such collisions.

   Note that, on Windows, the return is a Microsoft-specific full name for the
   zone, not an abbreviation.

   Parameter dstIndex must be 1 for DST or 0 for standard time.
   Returns the relevant form of the name of local-time's zone.
*/
QString qTzName(int dstIndex)
{
    Q_DECL_UNINITIALIZED
    char name[512];
    bool ok;
    size_t size = 0;
    {
        const auto locker = qt_scoped_lock(environmentMutex);
#if defined(_UCRT)  // i.e., MSVC and MinGW-UCRT
        ok = _get_tzname(&size, name, sizeof(name), dstIndex) == 0;
        size -= 1; // It includes space for '\0' (or !ok => we're going to ignore it).
        Q_ASSERT(!ok || size < sizeof(name));
#else
        const char *const src = tzname[dstIndex];
        size = src ? strlen(src) : 0;
        ok = src != nullptr && size < sizeof(name);
        if (ok)
            memcpy(name, src, size + 1);
#endif // _UCRT
    }
    return ok ? QString::fromLocal8Bit(name, qsizetype(size)) : QString();
}

QT_END_NAMESPACE
