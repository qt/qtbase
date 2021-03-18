/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2017 Intel Corporation.
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

#include "qtemporaryfile.h"

#include "qplatformdefs.h"
#include "qrandom.h"
#include "private/qtemporaryfile_p.h"
#include "private/qfile_p.h"
#include "private/qsystemerror_p.h"

#if !defined(Q_OS_WIN)
#include "private/qcore_unix_p.h"       // overrides QT_OPEN
#include <errno.h>
#endif

#if defined(QT_BUILD_CORE_LIB)
#include "qcoreapplication.h"
#else
#define tr(X) QString::fromLatin1(X)
#endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WIN)
typedef ushort Char;

static inline Char Latin1Char(char ch)
{
    return ushort(uchar(ch));
}

typedef HANDLE NativeFileHandle;

#else // POSIX
typedef char Char;
typedef char Latin1Char;
typedef int NativeFileHandle;
#endif

QTemporaryFileName::QTemporaryFileName(const QString &templateName)
{
    // Ensure there is a placeholder mask
    QString qfilename = templateName;
    uint phPos = qfilename.length();
    uint phLength = 0;

    while (phPos != 0) {
        --phPos;

        if (qfilename[phPos] == QLatin1Char('X')) {
            ++phLength;
            continue;
        }

        if (phLength >= 6
                || qfilename[phPos] == QLatin1Char('/')) {
            ++phPos;
            break;
        }

        // start over
        phLength = 0;
    }

    if (phLength < 6)
        qfilename.append(QLatin1String(".XXXXXX"));

    // "Nativify" :-)
    QFileSystemEntry::NativePath filename = QFileSystemEngine::absoluteName(
            QFileSystemEntry(qfilename, QFileSystemEntry::FromInternalPath()))
        .nativeFilePath();

    // Find mask in native path
    phPos = filename.length();
    phLength = 0;
    while (phPos != 0) {
        --phPos;

        if (filename[phPos] == Latin1Char('X')) {
            ++phLength;
            continue;
        }

        if (phLength >= 6) {
            ++phPos;
            break;
        }

        // start over
        phLength = 0;
    }

    Q_ASSERT(phLength >= 6);
    path = filename;
    pos = phPos;
    length = phLength;
}

/*!
    \internal

    Generates a unique file path from the template \a templ and returns it.
    The path in \c templ.path is modified.
*/
QFileSystemEntry::NativePath QTemporaryFileName::generateNext()
{
    Q_ASSERT(length != 0);
    Q_ASSERT(pos < path.length());
    Q_ASSERT(length <= path.length() - pos);

    Char *const placeholderStart = (Char *)path.data() + pos;
    Char *const placeholderEnd = placeholderStart + length;

    // Replace placeholder with random chars.
    {
        // Since our dictionary is 26+26 characters, it would seem we only need
        // a random number from 0 to 63 to select a character. However, due to
        // the limited range, that would mean 12 (64-52) characters have double
        // the probability of the others: 1 in 32 instead of 1 in 64.
        //
        // To overcome this limitation, we use more bits per character. With 10
        // bits, there are 16 characters with probability 19/1024 and the rest
        // at 20/1024 (i.e, less than .1% difference). This allows us to do 3
        // characters per 32-bit random number, which is also half the typical
        // placeholder length.
        enum { BitsPerCharacter = 10 };

        Char *rIter = placeholderEnd;
        while (rIter != placeholderStart) {
            quint32 rnd = QRandomGenerator::global()->generate();
            auto applyOne = [&]() {
                quint32 v = rnd & ((1 << BitsPerCharacter) - 1);
                rnd >>= BitsPerCharacter;
                char ch = char((26 + 26) * v / (1 << BitsPerCharacter));
                if (ch < 26)
                    *--rIter = Latin1Char(ch + 'A');
                else
                    *--rIter = Latin1Char(ch - 26 + 'a');
            };

            applyOne();
            if (rIter == placeholderStart)
                break;

            applyOne();
            if (rIter == placeholderStart)
                break;

            applyOne();
        }
    }

    return path;
}

#ifndef QT_NO_TEMPORARYFILE

/*!
    \internal

    Generates a unique file path from the template \a templ and creates a new
    file based based on those parameters: the \c templ.length characters in \c
    templ.path starting at \c templ.pos will be replacd by a random sequence of
    characters. \a mode specifies the file mode bits (not used on Windows).

    Returns true on success and sets the file handle on \a file. On error,
    returns false, sets an invalid handle on \a handle and sets the error
    condition in \a error. In both cases, the string in \a templ will be
    changed and contain the generated path name.
*/
static bool createFileFromTemplate(NativeFileHandle &file, QTemporaryFileName &templ,
                                   quint32 mode, int flags, QSystemError &error)
{
    const int maxAttempts = 16;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        // Atomically create file and obtain handle
        const QFileSystemEntry::NativePath &path = templ.generateNext();

#if defined(Q_OS_WIN)
        Q_UNUSED(mode);
        const DWORD shareMode = (flags & QTemporaryFileEngine::Win32NonShared)
                                ? 0u : (FILE_SHARE_READ | FILE_SHARE_WRITE);

#  ifndef Q_OS_WINRT
        file = CreateFile((const wchar_t *)path.constData(),
                GENERIC_READ | GENERIC_WRITE,
                shareMode, NULL, CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL, NULL);
#  else // !Q_OS_WINRT
        file = CreateFile2((const wchar_t *)path.constData(),
                GENERIC_READ | GENERIC_WRITE,
                shareMode, CREATE_NEW,
                NULL);
#  endif // Q_OS_WINRT

        if (file != INVALID_HANDLE_VALUE)
            return true;

        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            WIN32_FILE_ATTRIBUTE_DATA attributes;
            if (!GetFileAttributesEx((const wchar_t *)path.constData(),
                                     GetFileExInfoStandard, &attributes)
                    || attributes.dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
                // Potential write error (read-only parent directory, etc.).
                error = QSystemError(err, QSystemError::NativeError);
                return false;
            } // else file already exists as a directory.
        } else if (err != ERROR_FILE_EXISTS) {
            error = QSystemError(err, QSystemError::NativeError);
            return false;
        }
#else // POSIX
        Q_UNUSED(flags)
        file = QT_OPEN(path.constData(),
                QT_OPEN_CREAT | QT_OPEN_EXCL | QT_OPEN_RDWR | QT_OPEN_LARGEFILE,
                static_cast<mode_t>(mode));

        if (file != -1)
            return true;

        int err = errno;
        if (err != EEXIST) {
            error = QSystemError(err, QSystemError::NativeError);
            return false;
        }
#endif
    }

    return false;
}

enum class CreateUnnamedFileStatus {
    Success = 0,
    NotSupported,
    OtherError
};

static CreateUnnamedFileStatus
createUnnamedFile(NativeFileHandle &file, QTemporaryFileName &tfn, quint32 mode, QSystemError *error)
{
#ifdef LINUX_UNNAMED_TMPFILE
    // first, check if we have /proc, otherwise can't make the file exist later
    // (no error message set, as caller will try regular temporary file)
    if (!qt_haveLinuxProcfs())
        return CreateUnnamedFileStatus::NotSupported;

    const char *p = ".";
    QByteArray::size_type lastSlash = tfn.path.lastIndexOf('/');
    if (lastSlash >= 0) {
        if (lastSlash == 0)
            lastSlash = 1;
        tfn.path[lastSlash] = '\0';
        p = tfn.path.data();
    }

    file = QT_OPEN(p, O_TMPFILE | QT_OPEN_RDWR | QT_OPEN_LARGEFILE,
            static_cast<mode_t>(mode));
    if (file != -1)
        return CreateUnnamedFileStatus::Success;

    if (errno == EOPNOTSUPP || errno == EISDIR) {
        // fs or kernel doesn't support O_TMPFILE, so
        // put the slash back so we may try a regular file
        if (lastSlash != -1)
            tfn.path[lastSlash] = '/';
        return CreateUnnamedFileStatus::NotSupported;
    }

    // real error
    *error = QSystemError(errno, QSystemError::NativeError);
    return CreateUnnamedFileStatus::OtherError;
#else
    Q_UNUSED(file);
    Q_UNUSED(tfn);
    Q_UNUSED(mode);
    Q_UNUSED(error);
    return CreateUnnamedFileStatus::NotSupported;
#endif
}

//************* QTemporaryFileEngine
QTemporaryFileEngine::~QTemporaryFileEngine()
{
    Q_D(QFSFileEngine);
    d->unmapAll();
    QFSFileEngine::close();
}

bool QTemporaryFileEngine::isReallyOpen() const
{
    Q_D(const QFSFileEngine);

    if (!((nullptr == d->fh) && (-1 == d->fd)
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

bool QTemporaryFileEngine::open(QIODevice::OpenMode openMode)
{
    Q_D(QFSFileEngine);
    Q_ASSERT(!isReallyOpen());

    openMode |= QIODevice::ReadWrite;

    if (!filePathIsTemplate)
        return QFSFileEngine::open(openMode);

    QTemporaryFileName tfn(templateName);

    QSystemError error;
#if defined(Q_OS_WIN)
    NativeFileHandle &file = d->fileHandle;
#else // POSIX
    NativeFileHandle &file = d->fd;
#endif

    CreateUnnamedFileStatus st = createUnnamedFile(file, tfn, fileMode, &error);
    if (st == CreateUnnamedFileStatus::Success) {
        unnamedFile = true;
        d->fileEntry.clear();
    } else if (st == CreateUnnamedFileStatus::NotSupported &&
               createFileFromTemplate(file, tfn, fileMode, flags, error)) {
        filePathIsTemplate = false;
        unnamedFile = false;
        d->fileEntry = QFileSystemEntry(tfn.path, QFileSystemEntry::FromNativePath());
    } else {
        setError(QFile::OpenError, error.toString());
        return false;
    }

#if !defined(Q_OS_WIN) || defined(Q_OS_WINRT)
    d->closeFileHandle = true;
#endif

    d->openMode = openMode;
    d->lastFlushFailed = false;
    d->tried_stat = 0;

    return true;
}

bool QTemporaryFileEngine::remove()
{
    Q_D(QFSFileEngine);
    // Since the QTemporaryFileEngine::close() does not really close the file,
    // we must explicitly call QFSFileEngine::close() before we remove it.
    d->unmapAll();
    QFSFileEngine::close();
    if (isUnnamedFile())
        return true;
    if (!filePathIsTemplate && QFSFileEngine::remove()) {
        d->fileEntry.clear();
        // If a QTemporaryFile is constructed using a template file path, the path
        // is generated in QTemporaryFileEngine::open() and then filePathIsTemplate
        // is set to false. If remove() and then open() are called on the same
        // QTemporaryFile, the path is not regenerated. Here we ensure that if the
        // file path was generated, it will be generated again in the scenario above.
        filePathIsTemplate = filePathWasTemplate;
        return true;
    }
    return false;
}

bool QTemporaryFileEngine::rename(const QString &newName)
{
    if (isUnnamedFile()) {
        bool ok = materializeUnnamedFile(newName, DontOverwrite);
        QFSFileEngine::close();
        return ok;
    }
    QFSFileEngine::close();
    return QFSFileEngine::rename(newName);
}

bool QTemporaryFileEngine::renameOverwrite(const QString &newName)
{
    if (isUnnamedFile()) {
        bool ok = materializeUnnamedFile(newName, Overwrite);
        QFSFileEngine::close();
        return ok;
    }
    QFSFileEngine::close();
    return QFSFileEngine::renameOverwrite(newName);
}

bool QTemporaryFileEngine::close()
{
    // Don't close the file, just seek to the front.
    seek(0);
    setError(QFile::UnspecifiedError, QString());
    return true;
}

QString QTemporaryFileEngine::fileName(QAbstractFileEngine::FileName file) const
{
    if (isUnnamedFile()) {
        if (file == LinkName) {
            // we know our file isn't (won't be) a symlink
            return QString();
        }

        // for all other cases, materialize the file
        const_cast<QTemporaryFileEngine *>(this)->materializeUnnamedFile(templateName, NameIsTemplate);
    }
    return QFSFileEngine::fileName(file);
}

bool QTemporaryFileEngine::materializeUnnamedFile(const QString &newName, QTemporaryFileEngine::MaterializationMode mode)
{
    Q_ASSERT(isUnnamedFile());

#ifdef LINUX_UNNAMED_TMPFILE
    Q_D(QFSFileEngine);
    const QByteArray src = "/proc/self/fd/" + QByteArray::number(d->fd);
    auto materializeAt = [=](const QFileSystemEntry &dst) {
        return ::linkat(AT_FDCWD, src, AT_FDCWD, dst.nativeFilePath(), AT_SYMLINK_FOLLOW) == 0;
    };
#else
    auto materializeAt = [](const QFileSystemEntry &) { return false; };
#endif

    auto success = [this](const QFileSystemEntry &entry) {
        filePathIsTemplate = false;
        unnamedFile = false;
        d_func()->fileEntry = entry;
        return true;
    };

    auto materializeAsTemplate = [=](const QString &newName) {
        QTemporaryFileName tfn(newName);
        static const int maxAttempts = 16;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            tfn.generateNext();
            QFileSystemEntry entry(tfn.path, QFileSystemEntry::FromNativePath());
            if (materializeAt(entry))
                return success(entry);
        }
        return false;
    };

    if (mode == NameIsTemplate) {
        if (materializeAsTemplate(newName))
            return true;
    } else {
        // Use linkat to materialize the file
        QFileSystemEntry dst(newName);
        if (materializeAt(dst))
            return success(dst);

        if (errno == EEXIST && mode == Overwrite) {
            // retry by first creating a temporary file in the right dir
            if (!materializeAsTemplate(templateName))
                return false;

            // then rename the materialized file to target (same as renameOverwrite)
            QFSFileEngine::close();
            return QFSFileEngine::renameOverwrite(newName);
        }
    }

    // failed
    setError(QFile::RenameError, QSystemError(errno, QSystemError::NativeError).toString());
    return false;
}

bool QTemporaryFileEngine::isUnnamedFile() const
{
#ifdef LINUX_UNNAMED_TMPFILE
    if (unnamedFile) {
        Q_ASSERT(d_func()->fileEntry.isEmpty());
        Q_ASSERT(filePathIsTemplate);
    }
    return unnamedFile;
#else
    return false;
#endif
}

//************* QTemporaryFilePrivate

QTemporaryFilePrivate::QTemporaryFilePrivate()
{
}

QTemporaryFilePrivate::QTemporaryFilePrivate(const QString &templateNameIn)
    : templateName(templateNameIn)
{
}

QTemporaryFilePrivate::~QTemporaryFilePrivate()
{
}

QAbstractFileEngine *QTemporaryFilePrivate::engine() const
{
    if (!fileEngine) {
        fileEngine.reset(new QTemporaryFileEngine(&templateName));
        resetFileEngine();
    }
    return fileEngine.get();
}

void QTemporaryFilePrivate::resetFileEngine() const
{
    if (!fileEngine)
        return;

    QTemporaryFileEngine *tef = static_cast<QTemporaryFileEngine *>(fileEngine.get());
    if (fileName.isEmpty())
        tef->initialize(templateName, 0600);
    else
        tef->initialize(fileName, 0600, false);
}

void QTemporaryFilePrivate::materializeUnnamedFile()
{
#ifdef LINUX_UNNAMED_TMPFILE
    if (!fileName.isEmpty() || !fileEngine)
        return;

    auto *tef = static_cast<QTemporaryFileEngine *>(fileEngine.get());
    fileName = tef->fileName(QAbstractFileEngine::DefaultName);
#endif
}

QString QTemporaryFilePrivate::defaultTemplateName()
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
    \inmodule QtCore
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

    \snippet code/src_corelib_io_qtemporaryfile.cpp 0

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

    \note On Linux, QTemporaryFile will attempt to create unnamed temporary
    files. If that succeeds, open() will return true but exists() will be
    false. If you call fileName() or any function that calls it,
    QTemporaryFile will give the file a name, so most applications will
    not see a difference.

    \sa QDir::tempPath(), QFile
*/

#ifdef QT_NO_QOBJECT
QTemporaryFile::QTemporaryFile()
    : QFile(*new QTemporaryFilePrivate)
{
}

QTemporaryFile::QTemporaryFile(const QString &templateName)
    : QFile(*new QTemporaryFilePrivate(templateName))
{
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
    : QTemporaryFile(nullptr)
{
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
    : QTemporaryFile(templateName, nullptr)
{
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
    : QFile(*new QTemporaryFilePrivate(templateName), parent)
{
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
   Returns \c true if the QTemporaryFile is in auto remove
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
    Sets the QTemporaryFile into auto-remove mode if \a b is \c true.

    Auto-remove is on by default.

    If you set this property to \c false, ensure the application provides a way
    to remove the file once it is no longer needed, including passing the
    responsibility on to another process. Always use the fileName() function to
    obtain the name and never try to guess the name that QTemporaryFile has
    generated.

    On some systems, if fileName() is not called before closing the file, the
    temporary file may be removed regardless of the state of this property.
    This behavior should not be relied upon, so application code should either
    call fileName() or leave the auto removal functionality enabled.

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
    auto tef = static_cast<QTemporaryFileEngine *>(d->fileEngine.get());
    if (tef && tef->isReallyOpen())
        const_cast<QTemporaryFilePrivate *>(d)->materializeUnnamedFile();

    if(d->fileName.isEmpty())
        return QString();
    return d->engine()->fileName(QAbstractFileEngine::DefaultName);
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
   template contains XXXXXX that will automatically be replaced with
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
}

/*!
    \internal

    This is just a simplified version of QFile::rename() because we know a few
    extra details about what kind of file we have. The documentation is hidden
    from the user because QFile::rename() should be enough.
*/
bool QTemporaryFile::rename(const QString &newName)
{
    Q_D(QTemporaryFile);
    auto tef = static_cast<QTemporaryFileEngine *>(d->fileEngine.get());
    if (!tef || !tef->isReallyOpen() || !tef->filePathWasTemplate)
        return QFile::rename(newName);

    unsetError();
    close();
    if (error() == QFile::NoError) {
        if (tef->rename(newName)) {
            unsetError();
            // engine was able to handle the new name so we just reset it
            tef->setFileName(newName);
            d->fileName = newName;
            return true;
        }

        d->setError(QFile::RenameError, tef->errorString());
    }
    return false;
}

/*!
  \fn QTemporaryFile *QTemporaryFile::createLocalFile(const QString &fileName)
  \overload
  \obsolete

  Use QTemporaryFile::createNativeFile(const QString &fileName) instead.
*/

/*!
  \fn QTemporaryFile *QTemporaryFile::createLocalFile(QFile &file)
  \obsolete

  Use QTemporaryFile::createNativeFile(QFile &file) instead.
*/

/*!
  \fn QTemporaryFile *QTemporaryFile::createNativeFile(const QString &fileName)
  \overload

  Works on the given \a fileName rather than an existing QFile
  object.
*/


/*!
  If \a file is not already a native file, then a QTemporaryFile is created
  in QDir::tempPath(), the contents of \a file is copied into it, and a pointer
  to the temporary file is returned. Does nothing and returns \c 0 if \a file
  is already a native file.

  For example:

  \snippet code/src_corelib_io_qtemporaryfile.cpp 1

  \sa QFileInfo::isNativePath()
*/

QTemporaryFile *QTemporaryFile::createNativeFile(QFile &file)
{
    if (QAbstractFileEngine *engine = file.d_func()->engine()) {
        if(engine->fileFlags(QAbstractFileEngine::FlagsMask) & QAbstractFileEngine::LocalDiskFlag)
            return nullptr; // native already
        //cache
        bool wasOpen = file.isOpen();
        qint64 old_off = 0;
        if(wasOpen)
            old_off = file.pos();
        else if (!file.open(QIODevice::ReadOnly))
            return nullptr;
        //dump data
        QTemporaryFile *ret = new QTemporaryFile;
        if (ret->open()) {
            file.seek(0);
            char buffer[1024];
            while (true) {
                qint64 len = file.read(buffer, 1024);
                if (len < 1)
                    break;
                ret->write(buffer, len);
            }
            ret->seek(0);
        } else {
            delete ret;
            ret = nullptr;
        }
        //restore
        if(wasOpen)
            file.seek(old_off);
        else
            file.close();
        //done
        return ret;
    }
    return nullptr;
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
    auto tef = static_cast<QTemporaryFileEngine *>(d->fileEngine.get());
    if (tef && tef->isReallyOpen()) {
        setOpenMode(flags);
        return true;
    }

    // reset the engine state so it creates a new, unique file name from the template;
    // equivalent to:
    //    delete d->fileEngine;
    //    d->fileEngine = 0;
    //    d->engine();
    d->resetFileEngine();

    if (QFile::open(flags)) {
        tef = static_cast<QTemporaryFileEngine *>(d->fileEngine.get());
        if (tef->isUnnamedFile())
            d->fileName.clear();
        else
            d->fileName = tef->fileName(QAbstractFileEngine::DefaultName);
        return true;
    }
    return false;
}

#endif // QT_NO_TEMPORARYFILE

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qtemporaryfile.cpp"
#endif
