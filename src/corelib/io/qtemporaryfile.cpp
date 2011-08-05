/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtemporaryfile.h"

#ifndef QT_NO_TEMPORARYFILE

#include "qplatformdefs.h"
#include "qabstractfileengine.h"
#include "private/qfile_p.h"
#include "private/qabstractfileengine_p.h"
#include "private/qfsfileengine_p.h"

#if !defined(Q_OS_WINCE)
#  include <errno.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#if defined(Q_OS_UNIX)
# include "private/qcore_unix_p.h"      // overrides QT_OPEN
#endif

#if defined(QT_BUILD_CORE_LIB)
#include "qcoreapplication.h"
#endif

QT_BEGIN_NAMESPACE

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*!
    \internal

    Generates a unique file path and returns a native handle to the open file.
    \a path is used as a template when generating unique paths,
    \a placeholderStart and \a placeholderEnd delimit the sub-string that will
    be randomized.

    Returns an open handle to the newly created file if successful, an invalid
    handle otherwise. In both cases, the string in \a path will be changed and
    contain the generated path name.
*/
static int createFileFromTemplate(char *const path,
        char *const placeholderStart, char *const placeholderEnd)
{
    Q_ASSERT(placeholderEnd > placeholderStart);

    // Initialize placeholder with random chars + PID.
    {
        char *rIter = placeholderEnd;

#if defined(QT_BUILD_CORE_LIB)
        quint64 pid = quint64(QCoreApplication::applicationPid());
        do {
            *--rIter = (pid % 10) + '0';
            pid /= 10;
        } while (rIter != placeholderStart && pid != 0);
#endif

        while (rIter != placeholderStart) {
            char ch = char((qrand() & 0xffff) % (26 + 26));
            if (ch < 26)
                *--rIter = ch + 'A';
            else
                *--rIter = ch - 26 + 'a';
        }
    }

    for (;;) {
        // Atomically create file and obtain handle
#ifndef Q_OS_WIN
        {
            int fd = QT_OPEN(path, QT_OPEN_CREAT | O_EXCL | QT_OPEN_RDWR | QT_OPEN_LARGEFILE, 0600);
            if (fd != -1)
                return fd;
            if (errno != EEXIST)
                return -1;
        }
#else
        if (!QFileInfo(QLatin1String(path)).exists())
            return 1;
#endif

        /* tricky little algorwwithm for backward compatibility */
        for (char *iter = placeholderStart;;) {
            // Character progression: [0-9] => 'a' ... 'z' => 'A' .. 'Z'
            // String progression: "ZZaiC" => "aabiC"
            if (*iter == 'Z') {
                *iter++ = 'a';
                if (iter == placeholderEnd)
                    return -1;
            } else {
                if (isdigit(*iter))
                    *iter = 'a';
                else if (*iter == 'z') /* inc from z to A */
                    *iter = 'A';
                else {
                    ++*iter;
                }
                break;
            }
        }
    }
    /*NOTREACHED*/
}

//************* QTemporaryFileEngine
class QTemporaryFileEngine : public QFSFileEngine
{
    Q_DECLARE_PRIVATE(QFSFileEngine)
public:
    QTemporaryFileEngine(const QString &file, bool fileIsTemplate = true)
        : QFSFileEngine(), filePathIsTemplate(fileIsTemplate)
    {
        Q_D(QFSFileEngine);
        d->fileEntry = QFileSystemEntry(file);

        if (!filePathIsTemplate)
            QFSFileEngine::setFileName(file);
    }

    ~QTemporaryFileEngine();

    bool isReallyOpen();
    void setFileName(const QString &file);
    void setFileTemplate(const QString &fileTemplate);

    bool open(QIODevice::OpenMode flags);
    bool remove();
    bool rename(const QString &newName);
    bool close();

    bool filePathIsTemplate;
};

QTemporaryFileEngine::~QTemporaryFileEngine()
{
    QFSFileEngine::close();
}

bool QTemporaryFileEngine::isReallyOpen()
{
    Q_D(QFSFileEngine);

    if (!((0 == d->fh) && (-1 == d->fd)
#if defined (Q_OS_SYMBIAN)
                && (0 == d->symbianFile.SubSessionHandle())
#endif
#if defined Q_OS_WIN
                && (INVALID_HANDLE_VALUE == d->fileHandle)
#endif
            ))
        return true;

    return false;

}

void QTemporaryFileEngine::setFileName(const QString &file)
{
    // Really close the file, so we don't leak
    QFSFileEngine::close();
    QFSFileEngine::setFileName(file);
}

void QTemporaryFileEngine::setFileTemplate(const QString &fileTemplate)
{
    Q_D(QFSFileEngine);
    if (filePathIsTemplate)
        d->fileEntry = QFileSystemEntry(fileTemplate);
}

bool QTemporaryFileEngine::open(QIODevice::OpenMode openMode)
{
    Q_D(QFSFileEngine);
    Q_ASSERT(!isReallyOpen());

    openMode |= QIODevice::ReadWrite;

    if (!filePathIsTemplate)
        return QFSFileEngine::open(openMode);

    QString qfilename = d->fileEntry.filePath();

    // Find placeholder string.
    uint phPos = qfilename.length();
    uint phLength = 0;

    while (phPos != 0) {
        --phPos;

        if (qfilename[phPos] == QLatin1Char('X')) {
            ++phLength;
            continue;
        }

        if (qfilename[phPos] == QLatin1Char('/')
                || phLength >= 6) {
            ++phPos;
            break;
        }

        phLength = 0;
    }

    QStringRef prefix, suffix;
    if (phLength < 6) {
        qfilename += QLatin1Char('.');
        prefix = QStringRef(&qfilename);
        phLength = 6;
    } else {
        prefix = qfilename.leftRef(phPos);
        suffix = qfilename.midRef(phPos + phLength);
    }

    QByteArray filename = prefix.toLocal8Bit();
    phPos = filename.length();
    if (suffix.isEmpty())
        filename.resize(phPos + phLength);
    else
        filename.insert(phPos + phLength, suffix.toLocal8Bit());

    char *path = filename.data();

#ifndef Q_OS_WIN
    int fd = createFileFromTemplate(path, path + phPos, path + phPos + phLength);
    if (fd != -1) {
        // First open the fd as an external file descriptor to
        // initialize the engine properly.
        if (QFSFileEngine::open(openMode, fd)) {

            // Allow the engine to close the handle even if it's "external".
            d->closeFileHandle = true;

            // Restore the file names (open() resets them).
            d->fileEntry = QFileSystemEntry(QString::fromLocal8Bit(path, filename.length())); //note that filename is NOT a native path
            filePathIsTemplate = false;
            return true;
        }

        QT_CLOSE(fd);
    }
    setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError, qt_error_string(errno));
    return false;
#else
    if (createFileFromTemplate(path, path + phPos, path + phPos + phLength) == -1) {
        return false;
    }

    QString template_ = d->fileEntry.filePath();
    d->fileEntry = QFileSystemEntry(QString::fromLocal8Bit(path, filename.length()));

    if (QFSFileEngine::open(openMode)) {
        filePathIsTemplate = false;
        return true;
    }

    d->fileEntry = QFileSystemEntry(template_);
    return false;
#endif
}

bool QTemporaryFileEngine::remove()
{
    Q_D(QFSFileEngine);
    // Since the QTemporaryFileEngine::close() does not really close the file,
    // we must explicitly call QFSFileEngine::close() before we remove it.
    QFSFileEngine::close();
    if (QFSFileEngine::remove()) {
        d->fileEntry.clear();
        return true;
    }
    return false;
}

bool QTemporaryFileEngine::rename(const QString &newName)
{
    QFSFileEngine::close();
    return QFSFileEngine::rename(newName);
}

bool QTemporaryFileEngine::close()
{
    // Don't close the file, just seek to the front.
    seek(0);
    setError(QFile::UnspecifiedError, QString());
    return true;
}

//************* QTemporaryFilePrivate
class QTemporaryFilePrivate : public QFilePrivate
{
    Q_DECLARE_PUBLIC(QTemporaryFile)

protected:
    QTemporaryFilePrivate();
    ~QTemporaryFilePrivate();

    QString defaultTemplateName() const;

    bool autoRemove;
    QString templateName;
};

QTemporaryFilePrivate::QTemporaryFilePrivate() : autoRemove(true)
{
}

QTemporaryFilePrivate::~QTemporaryFilePrivate()
{
}

QString QTemporaryFilePrivate::defaultTemplateName() const
{
    QString baseName;
#if defined(QT_BUILD_CORE_LIB)
    baseName = QCoreApplication::applicationName();
    if (baseName.isEmpty())
#endif
        baseName = QLatin1String("qt_temp");

    return QDir::tempPath() + QLatin1Char('/') + baseName + QLatin1String(".XXXXXX");
}

//************* QTemporaryFile

/*!
    \class QTemporaryFile
    \reentrant
    \brief The QTemporaryFile class is an I/O device that operates on temporary files.

    \ingroup io


    QTemporaryFile is used to create unique temporary files safely.
    The file itself is created by calling open(). The name of the
    temporary file is guaranteed to be unique (i.e., you are
    guaranteed to not overwrite an existing file), and the file will
    subsequently be removed upon destruction of the QTemporaryFile
    object. This is an important technique that avoids data
    corruption for applications that store data in temporary files.
    The file name is either auto-generated, or created based on a
    template, which is passed to QTemporaryFile's constructor.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtemporaryfile.cpp 0

    Reopening a QTemporaryFile after calling close() is safe. For as long as
    the QTemporaryFile object itself is not destroyed, the unique temporary
    file will exist and be kept open internally by QTemporaryFile.

    The file name of the temporary file can be found by calling fileName().
    Note that this is only defined after the file is first opened; the function
    returns an empty string before this.

    A temporary file will have some static part of the name and some
    part that is calculated to be unique. The default filename will be
    determined from QCoreApplication::applicationName() (otherwise \c qt_temp) and will
    be placed into the temporary path as returned by QDir::tempPath().
    If you specify your own filename, a relative file path will not be placed in the
    temporary directory by default, but be relative to the current working directory.

    Specified filenames can contain the following template \c XXXXXX
    (six upper case "X" characters), which will be replaced by the
    auto-generated portion of the filename. Note that the template is
    case sensitive. If the template is not present in the filename,
    QTemporaryFile appends the generated part to the filename given.

    \sa QDir::tempPath(), QFile
*/

#ifdef QT_NO_QOBJECT
QTemporaryFile::QTemporaryFile()
    : QFile(*new QTemporaryFilePrivate)
{
    Q_D(QTemporaryFile);
    d->templateName = d->defaultTemplateName();
}

QTemporaryFile::QTemporaryFile(const QString &templateName)
    : QFile(*new QTemporaryFilePrivate)
{
    Q_D(QTemporaryFile);
    d->templateName = templateName;
}

#else
/*!
    Constructs a QTemporaryFile using as file template
    the application name returned by QCoreApplication::applicationName()
    (otherwise \c qt_temp) followed by ".XXXXXX".
    The file is stored in the system's temporary directory, QDir::tempPath().

    \sa setFileTemplate(), QDir::tempPath()
*/
QTemporaryFile::QTemporaryFile()
    : QFile(*new QTemporaryFilePrivate, 0)
{
    Q_D(QTemporaryFile);
    d->templateName = d->defaultTemplateName();
}

/*!
    Constructs a QTemporaryFile with a template filename of \a
    templateName. Upon opening the temporary file this will be used to create
    a unique filename.

    If the \a templateName does not contain XXXXXX it will automatically be
    appended and used as the dynamic portion of the filename.

    If \a templateName is a relative path, the path will be relative to the
    current working directory. You can use QDir::tempPath() to construct \a
    templateName if you want use the system's temporary directory.

    \sa open(), fileTemplate()
*/
QTemporaryFile::QTemporaryFile(const QString &templateName)
    : QFile(*new QTemporaryFilePrivate, 0)
{
    Q_D(QTemporaryFile);
    d->templateName = templateName;
}

/*!
    Constructs a QTemporaryFile (with the given \a parent)
    using as file template the application name returned by QCoreApplication::applicationName()
    (otherwise \c qt_temp) followed by ".XXXXXX".
    The file is stored in the system's temporary directory, QDir::tempPath().

    \sa setFileTemplate()
*/
QTemporaryFile::QTemporaryFile(QObject *parent)
    : QFile(*new QTemporaryFilePrivate, parent)
{
    Q_D(QTemporaryFile);
    d->templateName = d->defaultTemplateName();
}

/*!
    Constructs a QTemporaryFile with a template filename of \a
    templateName and the specified \a parent.
    Upon opening the temporary file this will be used to
    create a unique filename.

    If the \a templateName does not contain XXXXXX it will automatically be
    appended and used as the dynamic portion of the filename.

    If \a templateName is a relative path, the path will be relative to the
    current working directory. You can use QDir::tempPath() to construct \a
    templateName if you want use the system's temporary directory.

    \sa open(), fileTemplate()
*/
QTemporaryFile::QTemporaryFile(const QString &templateName, QObject *parent)
    : QFile(*new QTemporaryFilePrivate, parent)
{
    Q_D(QTemporaryFile);
    d->templateName = templateName;
}
#endif

/*!
    Destroys the temporary file object, the file is automatically
    closed if necessary and if in auto remove mode it will
    automatically delete the file.

    \sa autoRemove()
*/
QTemporaryFile::~QTemporaryFile()
{
    Q_D(QTemporaryFile);
    close();
    if (!d->fileName.isEmpty() && d->autoRemove)
        remove();
}

/*!
  \fn bool QTemporaryFile::open()

  A QTemporaryFile will always be opened in QIODevice::ReadWrite mode,
  this allows easy access to the data in the file. This function will
  return true upon success and will set the fileName() to the unique
  filename used.

  \sa fileName()
*/

/*!
   Returns true if the QTemporaryFile is in auto remove
   mode. Auto-remove mode will automatically delete the filename from
   disk upon destruction. This makes it very easy to create your
   QTemporaryFile object on the stack, fill it with data, read from
   it, and finally on function return it will automatically clean up
   after itself.

   Auto-remove is on by default.

   \sa setAutoRemove(), remove()
*/
bool QTemporaryFile::autoRemove() const
{
    Q_D(const QTemporaryFile);
    return d->autoRemove;
}

/*!
    Sets the QTemporaryFile into auto-remove mode if \a b is true.

    Auto-remove is on by default.

    \sa autoRemove(), remove()
*/
void QTemporaryFile::setAutoRemove(bool b)
{
    Q_D(QTemporaryFile);
    d->autoRemove = b;
}

/*!
   Returns the complete unique filename backing the QTemporaryFile
   object. This string is null before the QTemporaryFile is opened,
   afterwards it will contain the fileTemplate() plus
   additional characters to make it unique.

   \sa fileTemplate()
*/

QString QTemporaryFile::fileName() const
{
    Q_D(const QTemporaryFile);
    if(d->fileName.isEmpty())
        return QString();
    return fileEngine()->fileName(QAbstractFileEngine::DefaultName);
}

/*!
  Returns the set file template. The default file template will be
  called qcoreappname.XXXXXX and be placed in QDir::tempPath().

  \sa setFileTemplate()
*/
QString QTemporaryFile::fileTemplate() const
{
    Q_D(const QTemporaryFile);
    return d->templateName;
}

/*!
   Sets the static portion of the file name to \a name. If the file
   template ends in XXXXXX that will automatically be replaced with
   the unique part of the filename, otherwise a filename will be
   determined automatically based on the static portion specified.

    If \a name contains a relative file path, the path will be relative to the
    current working directory. You can use QDir::tempPath() to construct \a
    name if you want use the system's temporary directory.

   \sa fileTemplate()
*/
void QTemporaryFile::setFileTemplate(const QString &name)
{
    Q_D(QTemporaryFile);
    d->templateName = name;
    if (d->fileEngine)
        static_cast<QTemporaryFileEngine*>(d->fileEngine)->setFileTemplate(name);
}

/*!
  \fn QTemporaryFile *QTemporaryFile::createLocalFile(const QString &fileName)
  \overload

  Works on the given \a fileName rather than an existing QFile
  object.
*/


/*!
  If \a file is not on a local disk, a temporary file is created
  on a local disk, \a file is copied into the temporary local file,
  and a pointer to the temporary local file is returned. If \a file
  is already on a local disk, a copy is not created and 0 is returned.
*/
QTemporaryFile *QTemporaryFile::createLocalFile(QFile &file)
{
    if (QAbstractFileEngine *engine = file.fileEngine()) {
        if(engine->fileFlags(QAbstractFileEngine::FlagsMask) & QAbstractFileEngine::LocalDiskFlag)
            return 0; //local already
        //cache
        bool wasOpen = file.isOpen();
        qint64 old_off = 0;
        if(wasOpen)
            old_off = file.pos();
        else
            file.open(QIODevice::ReadOnly);
        //dump data
        QTemporaryFile *ret = new QTemporaryFile;
        ret->open();
        file.seek(0);
        char buffer[1024];
        while(true) {
            qint64 len = file.read(buffer, 1024);
            if(len < 1)
                break;
            ret->write(buffer, len);
        }
        ret->seek(0);
        //restore
        if(wasOpen)
            file.seek(old_off);
        else
            file.close();
        //done
        return ret;
    }
    return 0;
}

/*!
   \internal
*/

QAbstractFileEngine *QTemporaryFile::fileEngine() const
{
    Q_D(const QTemporaryFile);
    if(!d->fileEngine) {
        if (d->fileName.isEmpty())
            d->fileEngine = new QTemporaryFileEngine(d->templateName);
        else
            d->fileEngine = new QTemporaryFileEngine(d->fileName, false);
    }
    return d->fileEngine;
}

/*!
   \reimp

    Creates a unique file name for the temporary file, and opens it.  You can
    get the unique name later by calling fileName(). The file is guaranteed to
    have been created by this function (i.e., it has never existed before).
*/
bool QTemporaryFile::open(OpenMode flags)
{
    Q_D(QTemporaryFile);
    if (!d->fileName.isEmpty()) {
        if (static_cast<QTemporaryFileEngine*>(fileEngine())->isReallyOpen()) {
            setOpenMode(flags);
            return true;
        }
    }

    if (QFile::open(flags)) {
        d->fileName = d->fileEngine->fileName(QAbstractFileEngine::DefaultName);
        return true;
    }
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_TEMPORARYFILE


