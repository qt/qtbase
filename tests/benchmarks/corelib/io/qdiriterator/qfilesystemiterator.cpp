/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*!
    \since 4.5
    \class QFileSystemIterator
    \brief The QFileSystemIterator class provides an iterator for directory entrylists.

    You can use QFileSystemIterator to navigate entries of a directory one at a time.
    It is similar to QDir::entryList() and QDir::entryInfoList(), but because
    it lists entries one at a time instead of all at once, it scales better
    and is more suitable for large directories. It also supports listing
    directory contents recursively, and following symbolic links. Unlike
    QDir::entryList(), QFileSystemIterator does not support sorting.

    The QFileSystemIterator constructor takes a QDir or a directory as
    argument. After construction, the iterator is located before the first
    directory entry. Here's how to iterate over all the entries sequentially:

    \snippet doc/src/snippets/code/src.corelib.io.qdiriterator.cpp 0

    The next() function returns the path to the next directory entry and
    advances the iterator. You can also call filePath() to get the current
    file path without advancing the iterator.  The fileName() function returns
    only the name of the file, similar to how QDir::entryList() works. You can
    also call fileInfo() to get a QFileInfo for the current entry.

    Unlike Qt's container iterators, QFileSystemIterator is uni-directional (i.e.,
    you cannot iterate directories in reverse order) and does not allow random
    access.

    QFileSystemIterator works with all supported file engines, and is implemented
    using QAbstractFileEngineIterator.

    \sa QDir, QDir::entryList(), QAbstractFileEngineIterator
*/

/*! \enum QFileSystemIterator::IteratorFlag

    This enum describes flags that you can combine to configure the behavior
    of QFileSystemIterator.

    \value NoIteratorFlags The default value, representing no flags. The
    iterator will return entries for the assigned path.

    \value Subdirectories List entries inside all subdirectories as well.

    \value FollowSymlinks When combined with Subdirectories, this flag
    enables iterating through all subdirectories of the assigned path,
    following all symbolic links. Symbolic link loops (e.g., "link" => "." or
    "link" => "..") are automatically detected and ignored.
*/

#include "qfilesystemiterator.h"

#include <QDebug>
#include <QtCore/qset.h>
#include <QtCore/qstack.h>
#include <QtCore/qvariant.h>

#ifdef Q_OS_WIN
#   include <windows.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <dirent.h>
#   include <errno.h>
#endif

QT_BEGIN_NAMESPACE

namespace QDirIteratorTest {

class QFileSystemIteratorPrivate
{
public:
    QFileSystemIteratorPrivate(const QString &path, const QStringList &nameFilters,
                        QDir::Filters filters, QFileSystemIterator::IteratorFlags flags);
    ~QFileSystemIteratorPrivate();

    void pushSubDirectory(const QByteArray &path);
    void advance();
    bool isAcceptable() const;
    bool shouldFollowDirectory(const QFileInfo &);
    //bool matchesFilters(const QAbstractFileEngineIterator *it) const;
    inline bool atEnd() const { return m_dirPaths.isEmpty(); }

#ifdef Q_OS_WIN
    QStack<HANDLE>   m_dirStructs;
    WIN32_FIND_DATA* m_entry;
    WIN32_FIND_DATA  m_fileSearchResult;
    bool             m_bFirstSearchResult;
#else
    QStack<DIR *> m_dirStructs;
    dirent *m_entry;
#endif

    QSet<QString> visitedLinks;
    QStack<QByteArray> m_dirPaths;
    QFileInfo fileInfo;
    QString currentFilePath;
    QFileSystemIterator::IteratorFlags iteratorFlags;
    QDir::Filters filters;
    QStringList nameFilters;

    enum { DontShowDir, ShowDotDotDir, ShowDotDir, ShowDir }
        m_currentDirShown, m_nextDirShown;

    QFileSystemIterator *q;

private:
    bool advanceHelper();  // returns true if we know we have something suitable
};

/*!
    \internal
*/
QFileSystemIteratorPrivate::QFileSystemIteratorPrivate(const QString &path,
    const QStringList &nameFilters, QDir::Filters filters,
    QFileSystemIterator::IteratorFlags flags)
  : iteratorFlags(flags)
{
    if (filters == QDir::NoFilter)
        filters = QDir::AllEntries;
    this->filters = filters;
    this->nameFilters = nameFilters;

    fileInfo.setFile(path);
    QString dir = fileInfo.isSymLink() ? fileInfo.canonicalFilePath() : path;
    pushSubDirectory(dir.toLocal8Bit());
    // skip to acceptable entry
    while (true) {
        if (atEnd())
            return;
        if (isAcceptable())
            return;
        if (advanceHelper())
            return;
    }
}

/*!
    \internal
*/
QFileSystemIteratorPrivate::~QFileSystemIteratorPrivate()
{
#ifdef Q_OS_WIN
    while (!m_dirStructs.isEmpty())
        ::FindClose(m_dirStructs.pop());
#else
    while (!m_dirStructs.isEmpty())
        ::closedir(m_dirStructs.pop());
#endif
}

#ifdef Q_OS_WIN
static bool isDotOrDotDot(const wchar_t* name)
{
    if (name[0] == L'.' && name[1] == 0)
        return true;
    if (name[0] == L'.' && name[1] == L'.' && name[2] == 0)
        return true;
    return false;
}
#else
static bool isDotOrDotDot(const char *name)
{
    if (name[0] == '.' && name[1] == 0)
        return true;
    if (name[0] == '.' && name[1] == '.' && name[2] == 0)
        return true;
    return false;
}
#endif

/*!
    \internal
*/
void QFileSystemIteratorPrivate::pushSubDirectory(const QByteArray &path)
{
/*
    if (iteratorFlags & QFileSystemIterator::FollowSymlinks) {
        if (fileInfo.filePath() != path)
            fileInfo.setFile(path);
        if (fileInfo.isSymLink()) {
            visitedLinks << fileInfo.canonicalFilePath();
        } else {
            visitedLinks << fileInfo.absoluteFilePath();
        }
    }
*/

#ifdef Q_OS_WIN
    wchar_t szSearchPath[MAX_PATH];
    QString::fromLatin1(path).toWCharArray(szSearchPath);
    wcscat(szSearchPath, L"\\*");
#ifndef Q_OS_WINRT
    HANDLE dir = FindFirstFile(szSearchPath, &m_fileSearchResult);
#else
    HANDLE dir = FindFirstFileEx(szSearchPath, FindExInfoStandard, &m_fileSearchResult,
                                 FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_LARGE_FETCH);
#endif
    m_bFirstSearchResult = true;
#else
    DIR *dir = ::opendir(path.constData());
    //m_entry = ::readdir(dir);
    //while (m_entry && isDotOrDotDot(m_entry->d_name))
    //    m_entry = ::readdir(m_dirStructs.top());
#endif
    m_dirStructs.append(dir);
    m_dirPaths.append(path);
    m_entry = 0;
    if (filters & QDir::Dirs)
        m_nextDirShown = ShowDir;
    else
        m_nextDirShown = DontShowDir;
    m_currentDirShown = DontShowDir;
}

/*!
    \internal
*/
bool QFileSystemIteratorPrivate::isAcceptable() const
{
    if (!m_entry)
        return false;
    return true;
}

/*!
    \internal
*/


void QFileSystemIteratorPrivate::advance()
{
    while (true) {
        if (advanceHelper())
            return;
        if (atEnd())
            return;
        if (isAcceptable())
            return;
    }
}

bool QFileSystemIteratorPrivate::advanceHelper()
{
    if (m_dirStructs.isEmpty())
        return true;

    //printf("ADV %d %d\n", int(m_currentDirShown), int(m_nextDirShown));

    if ((filters & QDir::Dirs)) {
        m_currentDirShown = m_nextDirShown;
        if (m_nextDirShown == ShowDir) {
            //printf("RESTING ON DIR %s %x\n", m_dirPaths.top().constData(), int(filters));
            m_nextDirShown = (filters & QDir::NoDotAndDotDot) ? DontShowDir : ShowDotDir;
            // skip start directory itself
            if (m_dirStructs.size() == 1 && m_currentDirShown == ShowDir)
                return advanceHelper();
            return true;
        }
        if (m_nextDirShown == ShowDotDir) {
            //printf("RESTING ON DOT %s %x\n", m_dirPaths.top().constData(), int(filters));
            m_nextDirShown = ShowDotDotDir;
            return true;
        }
        if (m_nextDirShown == ShowDotDotDir) {
            //printf("RESTING ON DOTDOT %s %x\n", m_dirPaths.top().constData(), int(filters));
            m_nextDirShown = DontShowDir;
            return true;
        }
        m_currentDirShown = DontShowDir;
    }

#ifdef Q_OS_WIN
    m_entry = &m_fileSearchResult;
    if (m_bFirstSearchResult) {
        m_bFirstSearchResult = false;
    } else {
        if (!FindNextFile(m_dirStructs.top(), m_entry))
            m_entry = 0;
    }

    while (m_entry && isDotOrDotDot(m_entry->cFileName))
        if (!FindNextFile(m_dirStructs.top(), m_entry))
            m_entry = 0;

    if (!m_entry) {
        m_dirPaths.pop();
        FindClose(m_dirStructs.pop());
        return false;
    }

    if (m_entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        QByteArray ba = m_dirPaths.top();
        ba += '\\';
        ba += QString::fromWCharArray(m_entry->cFileName);
        pushSubDirectory(ba);
    }
#else
    m_entry = ::readdir(m_dirStructs.top());
    while (m_entry && isDotOrDotDot(m_entry->d_name))
        m_entry = ::readdir(m_dirStructs.top());
        //return false; // further iteration possibly needed
    //printf("READ %p %s\n", m_entry, m_entry ? m_entry->d_name : "");

    if (!m_entry) {
        m_dirPaths.pop();
        DIR *dir = m_dirStructs.pop();
        ::closedir(dir);
        return false; // further iteration possibly needed
    }

    const char *name = m_entry->d_name;

    QByteArray ba = m_dirPaths.top();
    ba += '/';
    ba += name;
    struct stat st;
    lstat(ba.constData(), &st);

    if (S_ISDIR(st.st_mode)) {
        pushSubDirectory(ba);
        return false; // further iteration possibly needed
    }
#endif
    return false; // further iteration possiblye needed
}

/*!
    \internal
 */
bool QFileSystemIteratorPrivate::shouldFollowDirectory(const QFileInfo &fileInfo)
{
    // If we're doing flat iteration, we're done.
    if (!(iteratorFlags & QFileSystemIterator::Subdirectories))
        return false;

    // Never follow non-directory entries
    if (!fileInfo.isDir())
        return false;


    // Never follow . and ..
    if (fileInfo.fileName() == QLatin1String(".") || fileInfo.fileName() == QLatin1String(".."))
        return false;


    // Check symlinks
    if (fileInfo.isSymLink() && !(iteratorFlags & QFileSystemIterator::FollowSymlinks)) {
        // Follow symlinks only if FollowSymlinks was passed
        return false;
    }

    // Stop link loops
    if (visitedLinks.contains(fileInfo.canonicalFilePath()))
        return false;

    return true;
}


/*!
    \internal

    This convenience function implements the iterator's filtering logics and
    applies then to the current directory entry.

    It returns true if the current entry matches the filters (i.e., the
    current entry will be returned as part of the directory iteration);
    otherwise, false is returned.
*/
#if 0
bool QFileSystemIteratorPrivate::matchesFilters(const QAbstractFileEngineIterator *it) const
{
    const bool filterPermissions = ((filters & QDir::PermissionMask)
                                    && (filters & QDir::PermissionMask) != QDir::PermissionMask);
    const bool skipDirs     = !(filters & (QDir::Dirs | QDir::AllDirs));
    const bool skipFiles    = !(filters & QDir::Files);
    const bool skipSymlinks = (filters & QDir::NoSymLinks);
    const bool doReadable   = !filterPermissions || (filters & QDir::Readable);
    const bool doWritable   = !filterPermissions || (filters & QDir::Writable);
    const bool doExecutable = !filterPermissions || (filters & QDir::Executable);
    const bool includeHidden = (filters & QDir::Hidden);
    const bool includeSystem = (filters & QDir::System);

#ifndef QT_NO_REGEXP
    // Prepare name filters
    QList<QRegExp> regexps;
    bool hasNameFilters = !nameFilters.isEmpty() && !(nameFilters.contains(QLatin1String("*")));
    if (hasNameFilters) {
        for (int i = 0; i < nameFilters.size(); ++i) {
            regexps << QRegExp(nameFilters.at(i),
                               (filters & QDir::CaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive,
                               QRegExp::Wildcard);
        }
    }
#endif

    QString fileName = it->currentFileName();
    if (fileName.isEmpty()) {
        // invalid entry
        return false;
    }

    QFileInfo fi = it->currentFileInfo();
    QString filePath = it->currentFilePath();

#ifndef QT_NO_REGEXP
    // Pass all entries through name filters, except dirs if the AllDirs
    // filter is passed.
    if (hasNameFilters && !((filters & QDir::AllDirs) && fi.isDir())) {
        bool matched = false;
        for (int i = 0; i < regexps.size(); ++i) {
            if (regexps.at(i).exactMatch(fileName)) {
                matched = true;
                break;
            }
        }
        if (!matched)
            return false;
    }
#endif

    bool dotOrDotDot = (fileName == QLatin1String(".") || fileName == QLatin1String(".."));
    if ((filters & QDir::NoDotAndDotDot) && dotOrDotDot)
        return false;

    bool isHidden = !dotOrDotDot && fi.isHidden();
    if (!includeHidden && isHidden)
        return false;

    bool isSystem = (!fi.isFile() && !fi.isDir() && !fi.isSymLink())
                    || (!fi.exists() && fi.isSymLink());
    if (!includeSystem && isSystem)
        return false;

    bool alwaysShow = (filters & QDir::TypeMask) == 0
        && ((isHidden && includeHidden)
            || (includeSystem && isSystem));

    // Skip files and directories
    if ((filters & QDir::AllDirs) == 0 && skipDirs && fi.isDir()) {
        if (!alwaysShow)
            return false;
    }

    if ((skipFiles && (fi.isFile() || !fi.exists()))
        || (skipSymlinks && fi.isSymLink())) {
        if (!alwaysShow)
            return false;
    }

    if (filterPermissions
        && ((doReadable && !fi.isReadable())
            || (doWritable && !fi.isWritable())
            || (doExecutable && !fi.isExecutable()))) {
        return false;
    }

    if (!includeSystem && !dotOrDotDot && ((fi.exists() && !fi.isFile() && !fi.isDir() && !fi.isSymLink())
                                           || (!fi.exists() && fi.isSymLink()))) {
        return false;
    }

    return true;
}
#endif

/*!
    Constructs a QFileSystemIterator that can iterate over \a dir's entrylist, using
    \a dir's name filters and regular filters. You can pass options via \a
    flags to decide how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as in QDir::entryList().

    The sorting in \a dir is ignored.

    \sa atEnd(), next(), IteratorFlags
*/
QFileSystemIterator::QFileSystemIterator(const QDir &dir, IteratorFlags flags)
    : d(new QFileSystemIteratorPrivate(dir.path(), dir.nameFilters(), dir.filter(), flags))
{
    d->q = this;
}

/*!
    Constructs a QFileSystemIterator that can iterate over \a path, with no name
    filtering and \a filters for entry filtering. You can pass options via \a
    flags to decide how the directory should be iterated.

    By default, \a filters is QDir::NoFilter, and \a flags is NoIteratorFlags,
    which provides the same behavior as in QDir::entryList().

    \sa atEnd(), next(), IteratorFlags
*/
QFileSystemIterator::QFileSystemIterator(const QString &path, QDir::Filters filters, IteratorFlags flags)
    : d(new QFileSystemIteratorPrivate(path, QStringList(QLatin1String("*")), filters, flags))
{
    d->q = this;
}

/*!
    Constructs a QFileSystemIterator that can iterate over \a path. You can pass
    options via \a flags to decide how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as in QDir::entryList().

    \sa atEnd(), next(), IteratorFlags
*/
QFileSystemIterator::QFileSystemIterator(const QString &path, IteratorFlags flags)
    : d(new QFileSystemIteratorPrivate(path, QStringList(QLatin1String("*")), QDir::NoFilter, flags))
{
    d->q = this;
}

/*!
    Constructs a QFileSystemIterator that can iterate over \a path, using \a
    nameFilters and \a filters. You can pass options via \a flags to decide
    how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as QDir::entryList().

    \sa atEnd(), next(), IteratorFlags
*/
QFileSystemIterator::QFileSystemIterator(const QString &path, const QStringList &nameFilters,
                           QDir::Filters filters, IteratorFlags flags)
    : d(new QFileSystemIteratorPrivate(path, nameFilters, filters, flags))
{
    d->q = this;
}

/*!
    Destroys the QFileSystemIterator.
*/
QFileSystemIterator::~QFileSystemIterator()
{
    delete d;
}

/*!
    Advances the iterator to the next entry, and returns the file path of this
    new entry. If atEnd() returns true, this function does nothing, and
    returns a null QString.

    You can call fileName() or filePath() to get the current entry file name
    or path, or fileInfo() to get a QFileInfo for the current entry.

    \sa hasNext(), fileName(), filePath(), fileInfo()
*/
void QFileSystemIterator::next()
{
    d->advance();
}

/*!
    Returns true if there is at least one more entry in the directory;
    otherwise, false is returned.

    \sa next(), fileName(), filePath(), fileInfo()
*/
bool QFileSystemIterator::atEnd() const
{
    return d->atEnd();
}

/*!
    Returns the file name for the current directory entry, without the path
    prepended. If the current entry is invalid (i.e., isValid() returns
    false), a null QString is returned.

    This function is provided for the convenience when iterating single
    directories. For recursive iteration, you should call filePath() or
    fileInfo() instead.

    \sa filePath(), fileInfo()
*/
QString QFileSystemIterator::fileName() const
{
    if (d->atEnd() || !d->m_entry)
        return QString();
    if (d->m_currentDirShown == QFileSystemIteratorPrivate::ShowDir)
        return QString();
    if (d->m_currentDirShown == QFileSystemIteratorPrivate::ShowDotDir)
        return QLatin1String("@");
    if (d->m_currentDirShown == QFileSystemIteratorPrivate::ShowDotDotDir)
        return QLatin1String("@@");
#ifdef Q_OS_WIN
    return QString::fromWCharArray(d->m_entry->cFileName);
#else
    return QString::fromLocal8Bit(d->m_entry->d_name);
#endif
}

/*!
    Returns the full file path for the current directory entry. If the current
    entry is invalid (i.e., isValid() returns false), a null QString is
    returned.

    \sa fileInfo(), fileName()
*/
QString QFileSystemIterator::filePath() const
{
    if (d->atEnd())
        return QString();
    QByteArray ba = d->m_dirPaths.top();
    if (d->m_currentDirShown == QFileSystemIteratorPrivate::ShowDotDir)
        ba += "/.";
    else if (d->m_currentDirShown == QFileSystemIteratorPrivate::ShowDotDotDir)
        ba += "/..";
    else if (d->m_entry) {
        ba += '/';
#ifdef Q_OS_WIN
        ba += QString::fromWCharArray(d->m_entry->cFileName);
#else
        ba += d->m_entry->d_name;
#endif
    }
    return QString::fromLocal8Bit(ba);
}

/*!
    Returns a QFileInfo for the current directory entry. If the current entry
    is invalid (i.e., isValid() returns false), a null QFileInfo is returned.

    \sa filePath(), fileName()
*/
QFileInfo QFileSystemIterator::fileInfo() const
{
    return QFileInfo(filePath());
}

/*!
    Returns the base directory of the iterator.
*/
QString QFileSystemIterator::path() const
{
    return QString::fromLocal8Bit(d->m_dirPaths.top());
}

} // QDirIteratorTest::

QT_END_NAMESPACE
