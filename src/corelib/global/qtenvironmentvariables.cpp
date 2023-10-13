// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtenvironmentvariables.h"
#include "qtenvironmentvariables_p.h"

#include <qplatformdefs.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>

#include <QtCore/private/qlocking_p.h>

QT_BEGIN_NAMESPACE

// In the C runtime on all platforms access to the environment is not thread-safe. We
// add thread-safety for the Qt wrappers.
Q_CONSTINIT static QBasicMutex environmentMutex;

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
    qEnvironmentVariableIsEmpty()
*/
QByteArray qgetenv(const char *varName)
{
    const auto locker = qt_scoped_lock(environmentMutex);
#ifdef Q_CC_MSVC
    size_t requiredSize = 0;
    QByteArray buffer;
    getenv_s(&requiredSize, 0, 0, varName);
    if (requiredSize == 0)
        return buffer;
    buffer.resize(qsizetype(requiredSize));
    getenv_s(&requiredSize, buffer.data(), requiredSize, varName);
    // requiredSize includes the terminating null, which we don't want.
    Q_ASSERT(buffer.endsWith('\0'));
    buffer.chop(1);
    return buffer;
#else
    return QByteArray(::getenv(varName));
#endif
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

    \sa qputenv(), qgetenv(), qEnvironmentVariableIsSet(), qEnvironmentVariableIsEmpty()
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
    QByteArray value = qgetenv(varName);
    if (value.isNull())
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
#ifdef Q_CC_MSVC
    // we provide a buffer that can only hold the empty string, so
    // when the env.var isn't empty, we'll get an ERANGE error (buffer
    // too small):
    size_t dummy;
    char buffer = '\0';
    return getenv_s(&dummy, &buffer, 1, varName) != ERANGE;
#else
    const char * const value = ::getenv(varName);
    return !value || !*value;
#endif
}

/*!
    \relates <QtEnvironmentVariables>
    \since 5.5

    Returns the numerical value of the environment variable \a varName.
    If \a ok is not null, sets \c{*ok} to \c true or \c false depending
    on the success of the conversion.

    Equivalent to
    \snippet code/src_corelib_global_qglobal.cpp to-int
    except that it's much faster, and can't throw exceptions.

    \note there's a limit on the length of the value, which is sufficient for
    all valid values of int, not counting leading zeroes or spaces. Values that
    are too long will either be truncated or this function will set \a ok to \c
    false.

    \sa qgetenv(), qEnvironmentVariable(), qEnvironmentVariableIsSet()
*/
int qEnvironmentVariableIntValue(const char *varName, bool *ok) noexcept
{
    static const int NumBinaryDigitsPerOctalDigit = 3;
    static const int MaxDigitsForOctalInt =
        (std::numeric_limits<uint>::digits + NumBinaryDigitsPerOctalDigit - 1) / NumBinaryDigitsPerOctalDigit;

    const auto locker = qt_scoped_lock(environmentMutex);
    size_t size;
#ifdef Q_CC_MSVC
    // we provide a buffer that can hold any int value:
    char buffer[MaxDigitsForOctalInt + 2]; // +1 for NUL +1 for optional '-'
    size_t dummy;
    if (getenv_s(&dummy, buffer, sizeof buffer, varName) != 0) {
        if (ok)
            *ok = false;
        return 0;
    }
    size = strlen(buffer);
#else
    const char * const buffer = ::getenv(varName);
    if (!buffer || (size = strlen(buffer)) > MaxDigitsForOctalInt + 2) {
        if (ok)
            *ok = false;
        return 0;
    }
#endif
    return QByteArrayView(buffer, size).toInt(ok, 0);
}

/*!
    \relates <QtEnvironmentVariables>
    \since 5.1

    Returns whether the environment variable \a varName is set.

    Equivalent to
    \snippet code/src_corelib_global_qglobal.cpp is-null
    except that it's potentially much faster, and can't throw exceptions.

    \sa qgetenv(), qEnvironmentVariable(), qEnvironmentVariableIsEmpty()
*/
bool qEnvironmentVariableIsSet(const char *varName) noexcept
{
    const auto locker = qt_scoped_lock(environmentMutex);
#ifdef Q_CC_MSVC
    size_t requiredSize = 0;
    (void)getenv_s(&requiredSize, 0, 0, varName);
    return requiredSize != 0;
#else
    return ::getenv(varName) != nullptr;
#endif
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

   Parameter dstIndex must be 1 for DST or 0 for standard time.
   Returns the relevant form of the name of local-time's zone.
*/
QString qTzName(int dstIndex)
{
    char name[512];
    bool ok;
#if defined(_UCRT)  // i.e., MSVC and MinGW-UCRT
    size_t s = 0;
    {
        const auto locker = qt_scoped_lock(environmentMutex);
        ok = _get_tzname(&s, name, 512, dstIndex) != 0;
    }
#else
    {
        const auto locker = qt_scoped_lock(environmentMutex);
        const char *const src = tzname[dstIndex];
        ok = src != nullptr;
        if (ok)
            memcpy(name, src, std::min(sizeof(name), strlen(src) + 1));
    }
#endif // Q_OS_WIN
    return ok ? QString::fromLocal8Bit(name) : QString();
}

QT_END_NAMESPACE
