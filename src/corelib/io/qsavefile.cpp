// Copyright (C) 2012 David Faure <faure@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:guaranteed-behavior

#include "qsavefile.h"

#if QT_CONFIG(temporaryfile)

#include "qplatformdefs.h"
#include "private/qsavefile_p.h"
#include "qfileinfo.h"
#include "qabstractfileengine_p.h"
#include <QtCore/qcoreapplication.h>
#include "qdebug.h"
#include "qtemporaryfile.h"
#include <QtCore/qttranslation.h>
#include "private/qiodevice_p.h"
#include "private/qtemporaryfile_p.h"
#ifdef Q_OS_UNIX
#include <errno.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QSaveFilePrivate::QSaveFilePrivate()
    : writeError(QFileDevice::NoError),
      useTemporaryFile(true),
      directWriteFallback(false)
{
}

QSaveFilePrivate::~QSaveFilePrivate()
{
}

bool QSaveFilePrivate::open(QIODevice::OpenMode mode)
{
    writeError = QFileDevice::NoError;
    if ((mode & (QIODevice::ReadOnly | QIODevice::WriteOnly)) == 0) {
        qWarning("QSaveFile::open: Open mode not specified");
        return false;
    }
    // In the future we could implement ReadWrite by copying from the existing file to the temp file...
    // The implications of NewOnly and ExistingOnly when used with QSaveFile need to be considered carefully...
    if (mode & (QIODevice::ReadOnly | QIODevice::Append | QIODevice::NewOnly
                | QIODevice::ExistingOnly)) {
        qWarning("QSaveFile::open: Unsupported open mode 0x%x", uint(mode.toInt()));
        return false;
    }

    // Check if existing file is writable:
    QFileInfo priorFile(fileName);
    if (!priorFile.isWritable() && priorFile.exists()) {
        setError(QFileDevice::WriteError,
                 QSaveFile::tr("Existing file %1 is not writable").arg(fileName));
        writeError = QFileDevice::WriteError;
        return false;
    }

    if (priorFile.isDir()) {
        setError(QFileDevice::WriteError, QSaveFile::tr("Filename refers to a directory"));
        writeError = QFileDevice::WriteError;
        return false;
    }
    // If the target file exists, and we haven't already been given other
    // permissions to use, save the existing permissions. For new files, see
    // below.
    if (!finalPermissions && priorFile.exists())
        finalPermissions = priorFile.permissions();
    // These may be overridden later by setPermissions(), of course.

    // Resolve symlinks. Don't use QFileInfo::canonicalFilePath so it still give
    // the expected target even if the file does not exist
    finalFileName = fileName;
    if (priorFile.isSymLink()) {
        int maxDepth = 128;
        while (--maxDepth && priorFile.isSymLink())
            priorFile.setFile(priorFile.symLinkTarget());
        if (maxDepth > 0)
            finalFileName = priorFile.filePath();
    }

    auto openDirectly = [this, mode]() {
        fileEngine = QAbstractFileEngine::create(finalFileName);
        if (fileEngine->open(mode | QIODevice::Unbuffered)) {
            useTemporaryFile = false;
            return true;
        }
        return false;
    };

    const char *directWriteReason = nullptr;
#ifdef Q_OS_WIN
    // check if it is an Alternate Data Stream
    if (finalFileName == fileName && fileName.indexOf(u':', 2) > 1)
        directWriteReason = QT_TRANSLATE_NOOP("QSaveFile", "target is an Alternate Data Stream");
#elif defined(Q_OS_ANDROID)
    // check if it is a content:// URL
    if (fileName.startsWith("content://"_L1))
        directWriteReason = QT_TRANSLATE_NOOP("QSaveFile", "target is a content:// virtual file");
#endif
    if (
#if defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
        !directWriteReason &&
#endif // Q_OS_WIN || Q_OS_ANDROID
        priorFile.exists() && !priorFile.isFile()) {
        directWriteReason = QT_TRANSLATE_NOOP("QSaveFile", "target exists and is not a regular file");
    }
    if (directWriteReason) {
        // yes, we can't rename onto it...
        if (directWriteFallback) {
            if (openDirectly())
                return true;
            setError(fileEngine->error(), fileEngine->errorString());
            fileEngine.reset();
        } else {
            setError(QFileDevice::OpenError,
                     QSaveFile::tr("QSaveFile cannot open '%1' "
                                   "without direct write fallback enabled: %2.")
                     .arg(QDir::toNativeSeparators(fileName),
                          QSaveFile::tr(directWriteReason)));
        }
        return false;
    }

    fileEngine.reset(new QTemporaryFileEngine(&finalFileName,
                                              QTemporaryFileEngine::Win32NonShared));
    // For new files, when other permissions haven't been specified, we want the
    // same permissions QFile::open() would get us. These depend on vagaries of
    // the operating system (Unix's umask(), for example) that we don't want to
    // second guess, so let open() do its thing and then read what it's done
    // before closing and reopening with 0600 for the real writing.
    if (!finalPermissions && QTemporaryFileEngine::CreatesWithFileMode) {
        Q_ASSERT(!priorFile.exists());
        // Dry-run of what follows, but with different permissions.
        static_cast<QTemporaryFileEngine *>(fileEngine.get())->initialize(finalFileName, 0666);
        if (fileEngine->open(mode | QIODevice::Unbuffered)) {
            finalPermissions = QFileDevicePrivate::permissions();
            fileEngine->close();
        }
        fileEngine->remove();
    }

    // We'll set the target file's permissions on commit() but, until then,
    // let's ensure the temporary file is not accessible to a third party.
    static_cast<QTemporaryFileEngine *>(fileEngine.get())->initialize(finalFileName, 0600);
    // Same as in QFile: QIODevice provides the buffering, so there's no need to
    // request it from the file engine.
    if (!fileEngine->open(mode | QIODevice::Unbuffered)) {
        QFileDevice::FileError err = fileEngine->error();
#ifdef Q_OS_UNIX
        if (directWriteFallback && err == QFileDevice::OpenError && errno == EACCES) {
            if (openDirectly())
                return true;
            err = fileEngine->error();
        }
#endif
        if (err == QFileDevice::UnspecifiedError)
            err = QFileDevice::OpenError;
        setError(err, fileEngine->errorString());
        fileEngine.reset();
        return false;
    }
    useTemporaryFile = true;
    return true;
}

QFileDevice::Permissions QSaveFilePrivate::permissions() const
{
    if (finalPermissions)
        return *finalPermissions;
    return QFileDevicePrivate::permissions();
}

bool QSaveFilePrivate::setPermissions(QFileDevice::Permissions perms)
{
    finalPermissions = perms;
    return true;
}

/*!
    \class QSaveFile
    \inmodule QtCore
    \brief The QSaveFile class provides an interface for safely writing to files.

    \ingroup io

    \reentrant

    \since 5.1

    QSaveFile is an I/O device for writing text and binary files, without losing
    existing data if the writing operation fails.

    While writing, the contents will be written to a temporary file, and if
    no error happened, commit() will move it to the final file. This ensures that
    no data at the final file is lost in case an error happens while writing,
    and no partially-written file is ever present at the final location. Always
    use QSaveFile when saving entire documents to disk.

    QSaveFile automatically detects errors while writing, such as the full partition
    situation, where write() cannot write all the bytes. It will remember that
    an error happened, and will discard the temporary file in commit().

    Much like with QFile, the file is opened with open(). Data is usually read
    and written using QDataStream or QTextStream, but you can also directly call
    \l write().

    Unlike QFile, calling close() is not allowed. commit() replaces it. If commit()
    was not called and the QSaveFile instance is destroyed, the temporary file is
    discarded.

    To abort saving due to an application error, call cancelWriting(), so that
    even a call to commit() later on will not save.

    \sa QTextStream, QDataStream, QFileInfo, QDir, QFile, QTemporaryFile
*/

/*!
    Constructs a new file object with the given \a parent.
    You need to call setFileName() before open().
*/
QSaveFile::QSaveFile(QObject *parent)
    : QFileDevice(*new QSaveFilePrivate, parent)
{
}

/*!
    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
QSaveFile::QSaveFile(const QString &name, QObject *parent)
    : QFileDevice(*new QSaveFilePrivate, parent)
{
    Q_D(QSaveFile);
    d->fileName = name;
}

/*!
    \fn QSaveFile::QSaveFile(const std::filesystem::path &path, QObject *parent)
    \since 6.11

    Constructs a new file object with the given \a parent to represent the
    file with the specified \a path.
*/

/*!
    Destroys the file object, discarding the saved contents unless commit() was called.
*/
QSaveFile::~QSaveFile()
{
    Q_D(QSaveFile);
    if (isOpen()) {
        QFileDevice::close();
        Q_ASSERT(d->fileEngine);
        d->fileEngine->remove();
    }
}

/*!
    Returns the name set by setFileName() or to the QSaveFile
    constructor.

    \sa setFileName()
*/
QString QSaveFile::fileName() const
{
    return d_func()->fileName;
}

/*!
    \fn std::filesystem::path QSaveFile::filesystemFileName() const
    \since 6.11
    Returns fileName() as \c{std::filesystem::path}.
*/

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute path.

    \sa QFile::setFileName(), fileName()
*/
void QSaveFile::setFileName(const QString &name)
{
    d_func()->fileName = name;
}

/*!
    \fn QSaveFile::setFileName(const std::filesystem::path &name)
    \since 6.11
    \overload
*/

/*!
    Opens the file using the given \a mode flags.

    Returns \c true if successful; otherwise returns \c false.

    Important: The flags for \a mode must include \l QIODeviceBase::WriteOnly. Other
    common flags you can use are \l Text and \l Unbuffered. Flags not supported at the
    moment are \l ReadOnly (and therefore \l ReadWrite), \l Append, \l NewOnly and \l ExistingOnly;
    they will generate a runtime warning.

    \sa setFileName(), QT_USE_NODISCARD_FILE_OPEN
*/
bool QSaveFile::open(OpenMode mode)
{
    Q_D(QSaveFile);
    if (isOpen()) {
        qWarning("QSaveFile::open: File (%ls) already open", qUtf16Printable(fileName()));
        return false;
    }
    unsetError();
    if (!d->open(mode))
        return false;
    return QFileDevice::open(mode);
}

/*!
  \reimp
  This method has been made private so that it cannot be called, in order to prevent mistakes.
  In order to finish writing the file, call commit().
  If instead you want to abort writing, call cancelWriting().
*/
void QSaveFile::close()
{
    qFatal("QSaveFile::close called");
}

/*!
    \fn bool QSaveFile::setPermissions(Permissions permissions)
    \reimp
    \since 6.12
    Sets the \a permissions the file shall be given on successful commit().

    While being written via QSaveFile the file may have more restrictive
    permissions.
*/

/*!
    \fn QFileDevice::Permissions QSaveFile::permissions() const
    \reimp
    \since 6.12
    Reports the permissions the file shall be given on successful commit().
*/

/*!
  Commits the changes to disk, if all previous writes were successful.

  It is mandatory to call this at the end of the saving operation, otherwise the file will be
  discarded.

  If an error happened during writing, deletes the temporary file and returns \c false.
  Otherwise, renames it to the final fileName and returns \c true on success.
  Finally, closes the device.

  \sa cancelWriting()
*/
bool QSaveFile::commit()
{
    Q_D(QSaveFile);
    if (!d->fileEngine)
        return false;

    if (!isOpen()) {
        qWarning("QSaveFile::commit: File (%ls) is not open", qUtf16Printable(fileName()));
        return false;
    }
    if (d->finalPermissions)
        d->QFileDevicePrivate::setPermissions(*d->finalPermissions); // Records error on failure.
    QFileDevice::close(); // calls flush()

    const auto &fe = d->fileEngine;

    // Sync to disk if possible. Ignore errors (e.g. not supported).
    fe->syncToDisk();

    // ensure we act on either a close()/flush() failure or a previous write()
    // problem
    if (d->error == QFileDevice::NoError)
        d->error = d->writeError;
    d->writeError = QFileDevice::NoError;

    if (d->useTemporaryFile) {
        if (d->error != QFileDevice::NoError) {
            fe->remove();
            return false;
        }
        // atomically replace old file with new file
        // Can't use QFile::rename for that, must use the file engine directly
        Q_ASSERT(fe);
        if (!fe->renameOverwrite(d->finalFileName)) {
            d->setError(fe->error(), fe->errorString());
            fe->remove();
            return false;
        }
    }

    // Return true if all previous write() calls succeeded and if close(),
    // flush() and (when relevant) setPermissions() succeeded.
    return d->error == QFileDevice::NoError;
}

/*!
  Cancels writing the new file.

  If the application changes its mind while saving, it can call cancelWriting(),
  which sets an error code so that commit() will discard the temporary file.

  Alternatively, it can simply make sure not to call commit().

  Further write operations are possible after calling this method, but none
  of it will have any effect, the written file will be discarded.

  This method has no effect when direct write fallback is used. This is the case
  when saving over an existing file in a readonly directory: no temporary file can
  be created, so the existing file is overwritten no matter what, and cancelWriting()
  cannot do anything about that, the contents of the existing file will be lost.

  \sa commit()
*/
void QSaveFile::cancelWriting()
{
    Q_D(QSaveFile);
    if (!isOpen())
        return;
    d->setError(QFileDevice::WriteError, QSaveFile::tr("Writing canceled by application"));
    d->writeError = QFileDevice::WriteError;
}

/*!
  \reimp
*/
qint64 QSaveFile::writeData(const char *data, qint64 len)
{
    Q_D(QSaveFile);
    if (d->writeError != QFileDevice::NoError)
        return -1;

    const qint64 ret = QFileDevice::writeData(data, len);

    if (d->error != QFileDevice::NoError)
        d->writeError = d->error;
    return ret;
}

/*!
  Allows writing over the existing file if necessary.

  QSaveFile creates a temporary file in the same directory as the final
  file and atomically renames it. However this is not possible if the
  directory permissions do not allow creating new files.
  In order to preserve atomicity guarantees, open() fails when it
  cannot create the temporary file.

  In order to allow users to edit files with write permissions in a
  directory with restricted permissions, call setDirectWriteFallback() with
  \a enabled set to true, and the following calls to open() will fallback to
  opening the existing file directly and writing into it, without the use of
  a temporary file.
  This does not have atomicity guarantees, i.e. an application crash or
  for instance a power failure could lead to a partially-written file on disk.
  It also means cancelWriting() has no effect, in such a case.

  Typically, to save documents edited by the user, call setDirectWriteFallback(true),
  and to save application internal files (configuration files, data files, ...), keep
  the default setting which ensures atomicity.

  \sa directWriteFallback()
*/
void QSaveFile::setDirectWriteFallback(bool enabled)
{
    Q_D(QSaveFile);
    d->directWriteFallback = enabled;
}

/*!
  Returns \c true if the fallback solution for saving files in read-only
  directories is enabled.

  \sa setDirectWriteFallback()
*/
bool QSaveFile::directWriteFallback() const
{
    Q_D(const QSaveFile);
    return d->directWriteFallback;
}

QT_END_NAMESPACE

#include "moc_qsavefile.cpp"

#endif // QT_CONFIG(temporaryfile)
