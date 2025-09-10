// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qfilesystementry_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/private/qfsfileengine_p.h>
#ifdef Q_OS_WIN
#include <QtCore/qstringbuilder.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Assigned to m_lastSeparator and m_firstDotInFileName to indicate resolveFilePath()
// hasn't been called yet
constexpr int Uninitialized = -2;

#ifdef Q_OS_WIN
static bool isUncRoot(const QString &server)
{
    QString localPath = QDir::toNativeSeparators(server);
    if (!localPath.startsWith("\\\\"_L1))
        return false;

    int idx = localPath.indexOf(u'\\', 2);
    if (idx == -1 || idx + 1 == localPath.length())
        return true;

    return QStringView{localPath}.right(localPath.length() - idx - 1).trimmed().isEmpty();
}

static inline QString fixIfRelativeUncPath(const QString &path)
{
    QString currentPath = QDir::currentPath();
    if (currentPath.startsWith("//"_L1))
        return currentPath % QChar(u'/') % path;
    return path;
}
#endif

QFileSystemEntry::QFileSystemEntry()
    : m_lastSeparator(-1),
    m_firstDotInFileName(-1),
    m_lastDotInFileName(-1)
{
}

/*!
   \internal
   Use this constructor when the path is supplied by user code, as it may contain a mix
   of '/' and the native separator.
 */
QFileSystemEntry::QFileSystemEntry(const QString &filePath)
    : m_filePath(QDir::fromNativeSeparators(filePath)),
    m_lastSeparator(Uninitialized),
    m_firstDotInFileName(Uninitialized),
    m_lastDotInFileName(0)
{
}

/*!
   \internal
   Use this constructor when the path is guaranteed to be in internal format, i.e. all
   directory separators are '/' and not the native separator.
 */
QFileSystemEntry::QFileSystemEntry(const QString &filePath, FromInternalPath /* dummy */)
    : m_filePath(filePath),
    m_lastSeparator(Uninitialized),
    m_firstDotInFileName(Uninitialized),
    m_lastDotInFileName(0)
{
}

/*!
   \internal
   Use this constructor when the path comes from a native API
 */
QFileSystemEntry::QFileSystemEntry(const NativePath &nativeFilePath, FromNativePath /* dummy */)
    : m_nativeFilePath(nativeFilePath),
    m_lastSeparator(Uninitialized),
    m_firstDotInFileName(Uninitialized),
    m_lastDotInFileName(0)
{
}

QFileSystemEntry::QFileSystemEntry(const QString &filePath, const NativePath &nativeFilePath)
    : m_filePath(QDir::fromNativeSeparators(filePath)),
    m_nativeFilePath(nativeFilePath),
    m_lastSeparator(Uninitialized),
    m_firstDotInFileName(Uninitialized),
    m_lastDotInFileName(0)
{
}

QString QFileSystemEntry::filePath() const
{
    resolveFilePath();
    return m_filePath;
}

QFileSystemEntry::NativePath QFileSystemEntry::nativeFilePath() const
{
    resolveNativeFilePath();
    return m_nativeFilePath;
}

void QFileSystemEntry::resolveFilePath() const
{
    if (m_filePath.isEmpty() && !m_nativeFilePath.isEmpty()) {
#ifdef Q_OS_WIN
        m_filePath = QDir::fromNativeSeparators(m_nativeFilePath);
#else
        m_filePath = QDir::fromNativeSeparators(QFile::decodeName(m_nativeFilePath));
#endif
    }
}

void QFileSystemEntry::resolveNativeFilePath() const
{
    if (!m_filePath.isEmpty() && m_nativeFilePath.isEmpty()) {
#ifdef Q_OS_WIN
        QString filePath = m_filePath;
        if (isRelative())
            filePath = fixIfRelativeUncPath(m_filePath);
        m_nativeFilePath = QFSFileEnginePrivate::longFileName(QDir::toNativeSeparators(filePath));
#else
        m_nativeFilePath = QFile::encodeName(QDir::toNativeSeparators(m_filePath));
#endif
    }
}

QString QFileSystemEntry::fileName() const
{
    findLastSeparator();
#if defined(Q_OS_WIN)
    if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == u':')
        return m_filePath.mid(2);
#endif
    return m_filePath.mid(m_lastSeparator + 1);
}

QString QFileSystemEntry::path() const
{
    findLastSeparator();
    if (m_lastSeparator == -1) {
#if defined(Q_OS_WIN)
        if (m_filePath.length() >= 2 && m_filePath.at(1) == u':')
            return m_filePath.left(2);
#endif
        return QString(u'.');
    }
    if (m_lastSeparator == 0)
        return QString(u'/');
#if defined(Q_OS_WIN)
    if (m_lastSeparator == 2 && m_filePath.at(1) == u':')
        return m_filePath.left(m_lastSeparator + 1);
#endif
    return m_filePath.left(m_lastSeparator);
}

QString QFileSystemEntry::baseName() const
{
    findFileNameSeparators();
    int length = -1;
    if (m_firstDotInFileName >= 0) {
        length = m_firstDotInFileName;
        if (m_lastSeparator != -1) // avoid off by one
            length--;
    }
#if defined(Q_OS_WIN)
    if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == u':')
        return m_filePath.mid(2, length - 2);
#endif
    return m_filePath.mid(m_lastSeparator + 1, length);
}

QString QFileSystemEntry::completeBaseName() const
{
    findFileNameSeparators();
    int length = -1;
    if (m_firstDotInFileName >= 0) {
        length = m_firstDotInFileName + m_lastDotInFileName;
        if (m_lastSeparator != -1) // avoid off by one
            length--;
    }
#if defined(Q_OS_WIN)
    if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == u':')
        return m_filePath.mid(2, length - 2);
#endif
    return m_filePath.mid(m_lastSeparator + 1, length);
}

QString QFileSystemEntry::suffix() const
{
    findFileNameSeparators();

    if (m_lastDotInFileName == -1)
        return QString();

    return m_filePath.mid(qMax((qint16)0, m_lastSeparator) + m_firstDotInFileName + m_lastDotInFileName + 1);
}

QString QFileSystemEntry::completeSuffix() const
{
    findFileNameSeparators();
    if (m_firstDotInFileName == -1)
        return QString();

    return m_filePath.mid(qMax((qint16)0, m_lastSeparator) + m_firstDotInFileName + 1);
}

#if defined(Q_OS_WIN)
bool QFileSystemEntry::isRelative() const
{
    resolveFilePath();
    return (m_filePath.isEmpty()
            || (m_filePath.at(0).unicode() != '/'
                && !(m_filePath.length() >= 2 && m_filePath.at(1).unicode() == ':')));
}

bool QFileSystemEntry::isAbsolute() const
{
    resolveFilePath();
    return ((m_filePath.length() >= 3
             && m_filePath.at(0).isLetter()
             && m_filePath.at(1).unicode() == ':'
             && m_filePath.at(2).unicode() == '/')
         || (m_filePath.length() >= 2
             && m_filePath.at(0) == u'/'
             && m_filePath.at(1) == u'/'));
}
#else
bool QFileSystemEntry::isRelative() const
{
    return !isAbsolute();
}

bool QFileSystemEntry::isAbsolute() const
{
    resolveFilePath();
    return (!m_filePath.isEmpty() && (m_filePath.at(0).unicode() == '/'));
}
#endif

#if defined(Q_OS_WIN)
bool QFileSystemEntry::isDriveRoot() const
{
    resolveFilePath();
    return QFileSystemEntry::isDriveRootPath(m_filePath);
}

bool QFileSystemEntry::isDriveRootPath(const QString &path)
{
    return (path.length() == 3
           && path.at(0).isLetter() && path.at(1) == u':'
           && path.at(2) == u'/');
}

QString QFileSystemEntry::removeUncOrLongPathPrefix(QString path)
{
    constexpr qsizetype minPrefixSize = 4;
    if (path.size() < minPrefixSize)
        return path;

    auto data = path.data();
    const auto slash = path[0];
    if (slash != u'\\' && slash != u'/')
        return path;

    // check for "//?/" or "/??/"
    if (data[2] == u'?' && data[3] == slash && (data[1] == slash || data[1] == u'?')) {
        path = path.sliced(minPrefixSize);

        // check for a possible "UNC/" prefix left-over
        if (path.size() >= 4) {
            data = path.data();
            if (data[0] == u'U' && data[1] == u'N' && data[2] == u'C' && data[3] == slash) {
                data[2] = slash;
                return path.sliced(2);
            }
        }
    }

    return path;
}
#endif // Q_OS_WIN

bool QFileSystemEntry::isRootPath(const QString &path)
{
    if (path == "/"_L1
#if defined(Q_OS_WIN)
            || isDriveRootPath(path)
            || isUncRoot(path)
#endif
            )
        return true;

    return false;
}

bool QFileSystemEntry::isRoot() const
{
    resolveFilePath();
    return isRootPath(m_filePath);
}

bool QFileSystemEntry::isEmpty() const
{
    return m_filePath.isEmpty() && m_nativeFilePath.isEmpty();
}

// private methods

void QFileSystemEntry::findLastSeparator() const
{
    if (m_lastSeparator == Uninitialized) {
        resolveFilePath();
        m_lastSeparator = m_filePath.lastIndexOf(u'/');
    }
}

void QFileSystemEntry::findFileNameSeparators() const
{
    if (m_firstDotInFileName == Uninitialized) {
        resolveFilePath();
        int firstDotInFileName = -1;
        int lastDotInFileName = -1;
        int lastSeparator = m_lastSeparator;

        int stop;
        if (lastSeparator < 0) {
            lastSeparator = -1;
            stop = 0;
        } else {
            stop = lastSeparator;
        }

        int i = m_filePath.size() - 1;
        for (; i >= stop; --i) {
            if (m_filePath.at(i).unicode() == '.') {
                firstDotInFileName = lastDotInFileName = i;
                break;
            } else if (m_filePath.at(i).unicode() == '/') {
                lastSeparator = i;
                break;
            }
        }

        if (lastSeparator != i) {
            for (--i; i >= stop; --i) {
                if (m_filePath.at(i).unicode() == '.')
                    firstDotInFileName = i;
                else if (m_filePath.at(i).unicode() == '/') {
                    lastSeparator = i;
                    break;
                }
            }
        }
        m_lastSeparator = lastSeparator;
        m_firstDotInFileName = firstDotInFileName == -1 ? -1 : firstDotInFileName - qMax(0, lastSeparator);
        if (lastDotInFileName == -1)
            m_lastDotInFileName = -1;
        else if (firstDotInFileName == lastDotInFileName)
            m_lastDotInFileName = 0;
        else
            m_lastDotInFileName = lastDotInFileName - firstDotInFileName;
    }
}

bool QFileSystemEntry::isClean() const
{
    resolveFilePath();
    int dots = 0;
    bool dotok = true; // checking for ".." or "." starts to relative paths
    bool slashok = true;
    for (QString::const_iterator iter = m_filePath.constBegin(); iter != m_filePath.constEnd(); ++iter) {
        if (*iter == u'/') {
            if (dots == 1 || dots == 2)
                return false; // path contains "./" or "../"
            if (!slashok)
                return false; // path contains "//"
            dots = 0;
            dotok = true;
            slashok = false;
        } else if (dotok) {
            slashok = true;
            if (*iter == u'.') {
                dots++;
                if (dots > 2)
                    dotok = false;
            } else {
                //path element contains a character other than '.', it's clean
                dots = 0;
                dotok = false;
            }
        }
    }
    return (dots != 1 && dots != 2); // clean if path doesn't end in . or ..
}

QT_END_NAMESPACE
