// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfileinfogatherer_p.h"
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdirlisting.h>
#include <private/qabstractfileiconprovider_p.h>
#include <private/qfileinfo_p.h>
#ifndef Q_OS_WIN
#  include <unistd.h>
#  include <sys/types.h>
#endif
#if defined(Q_OS_VXWORKS)
#  include "qplatformdefs.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifdef QT_BUILD_INTERNAL
Q_CONSTINIT static QBasicAtomicInt fetchedRoot = Q_BASIC_ATOMIC_INITIALIZER(false);
Q_AUTOTEST_EXPORT void qt_test_resetFetchedRoot()
{
    fetchedRoot.storeRelaxed(false);
}

Q_AUTOTEST_EXPORT bool qt_test_isFetchedRoot()
{
    return fetchedRoot.loadRelaxed();
}
#endif

static QString translateDriveName(const QFileInfo &drive)
{
    QString driveName = drive.absoluteFilePath();
#ifdef Q_OS_WIN
    if (driveName.startsWith(u'/')) // UNC host
        return drive.fileName();
    if (driveName.endsWith(u'/'))
        driveName.chop(1);
#endif // Q_OS_WIN
    return driveName;
}

/*!
    Creates thread
*/
QFileInfoGatherer::QFileInfoGatherer(QObject *parent)
    : QThread(parent)
    , m_iconProvider(&defaultProvider)
{
    start(LowPriority);
}

/*!
    Destroys thread
*/
QFileInfoGatherer::~QFileInfoGatherer()
{
    requestAbort();
    wait();
}

bool QFileInfoGatherer::event(QEvent *event)
{
    if (event->type() == QEvent::DeferredDelete && isRunning()) {
        // We have been asked to shut down later but were blocked,
        // so the owning QFileSystemModel proceeded with its shut-down
        // and deferred the destruction of the gatherer.
        // If we are still blocked now, then we have three bad options:
        // terminate, wait forever (preventing the process from shutting down),
        // or accept a memory leak.
        requestAbort();
        if (!wait(5000)) {
            // If the application is shutting down, then we terminate.
            // Otherwise assume that sooner or later the thread will finish,
            // and we delete it then.
            if (QCoreApplication::closingDown())
                terminate();
            else
                connect(this, &QThread::finished, this, [this]{ delete this; });
            return true;
        }
    }

    return QThread::event(event);
}

void QFileInfoGatherer::requestAbort()
{
    requestInterruption();
    QMutexLocker locker(&mutex);
    condition.wakeAll();
}

void QFileInfoGatherer::setResolveSymlinks(bool enable)
{
    Q_UNUSED(enable);
#ifdef Q_OS_WIN
    m_resolveSymlinks = enable;
#endif
}

void QFileInfoGatherer::driveAdded()
{
    fetchExtendedInformation(QString(), QStringList());
}

void QFileInfoGatherer::driveRemoved()
{
    QStringList drives;
    const QFileInfoList driveInfoList = QDir::drives();
    for (const QFileInfo &fi : driveInfoList)
        drives.append(translateDriveName(fi));
    emit newListOfFiles(QString(), drives);
}

bool QFileInfoGatherer::resolveSymlinks() const
{
#ifdef Q_OS_WIN
    return m_resolveSymlinks;
#else
    return false;
#endif
}

void QFileInfoGatherer::setIconProvider(QAbstractFileIconProvider *provider)
{
    m_iconProvider = provider;
}

QAbstractFileIconProvider *QFileInfoGatherer::iconProvider() const
{
    return m_iconProvider;
}

/*!
    Fetch extended information for all \a files in \a path

    \sa updateFile(), update(), resolvedName()
*/
void QFileInfoGatherer::fetchExtendedInformation(const QString &path, const QStringList &files)
{
    QMutexLocker locker(&mutex);
    // See if we already have this dir/file in our queue
    qsizetype loc = 0;
    while ((loc = this->path.lastIndexOf(path, loc - 1)) != -1) {
        if (this->files.at(loc) == files)
            return;
        if (loc == 0)
            break;
    }

#if QT_CONFIG(thread)
    this->path.push(path);
    this->files.push(files);
    condition.wakeAll();
#else // !QT_CONFIG(thread)
    getFileInfos(path, files);
#endif // QT_CONFIG(thread)

#if QT_CONFIG(filesystemwatcher)
    if (files.isEmpty()
        && !path.isEmpty()
        && !path.startsWith("//"_L1) /*don't watch UNC path*/) {
        if (!watchedDirectories().contains(path))
            watchPaths(QStringList(path));
    }
#endif
}

/*!
    Fetch extended information for all \a filePath

    \sa fetchExtendedInformation()
*/
void QFileInfoGatherer::updateFile(const QString &filePath)
{
    QString dir = filePath.mid(0, filePath.lastIndexOf(u'/'));
    QString fileName = filePath.mid(dir.size() + 1);
    fetchExtendedInformation(dir, QStringList(fileName));
}

QStringList QFileInfoGatherer::watchedFiles() const
{
#if QT_CONFIG(filesystemwatcher)
    if (m_watcher)
        return m_watcher->files();
#endif
    return {};
}

QStringList QFileInfoGatherer::watchedDirectories() const
{
#if QT_CONFIG(filesystemwatcher)
    if (m_watcher)
        return m_watcher->directories();
#endif
    return {};
}

void QFileInfoGatherer::createWatcher()
{
#if QT_CONFIG(filesystemwatcher)
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &QFileInfoGatherer::list);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &QFileInfoGatherer::updateFile);
#  if defined(Q_OS_WIN)
    const QVariant listener = m_watcher->property("_q_driveListener");
    if (listener.canConvert<QObject *>()) {
        if (QObject *driveListener = listener.value<QObject *>()) {
            connect(driveListener, SIGNAL(driveAdded()), this, SLOT(driveAdded()));
            connect(driveListener, SIGNAL(driveRemoved()), this, SLOT(driveRemoved()));
        }
    }
#  endif // Q_OS_WIN
#endif
}

void QFileInfoGatherer::watchPaths(const QStringList &paths)
{
#if QT_CONFIG(filesystemwatcher)
    if (m_watching) {
        if (m_watcher == nullptr)
            createWatcher();
        m_watcher->addPaths(paths);
    }
#else
    Q_UNUSED(paths);
#endif
}

void QFileInfoGatherer::unwatchPaths(const QStringList &paths)
{
#if QT_CONFIG(filesystemwatcher)
    if (m_watcher && !paths.isEmpty())
        m_watcher->removePaths(paths);
#else
    Q_UNUSED(paths);
#endif
}

bool QFileInfoGatherer::isWatching() const
{
    bool result = false;
#if QT_CONFIG(filesystemwatcher)
    QMutexLocker locker(&mutex);
    result = m_watching;
#endif
    return result;
}

/*! \internal

    If \a v is \c false, the QFileSystemWatcher used internally will be deleted
    and subsequent calls to watchPaths() will do nothing.

    If \a v is \c true, subsequent calls to watchPaths() will add those paths to
    the filesystem watcher; watchPaths() will initialize a QFileSystemWatcher if
    one hasn't already been initialized.
*/
void QFileInfoGatherer::setWatching(bool v)
{
#if QT_CONFIG(filesystemwatcher)
    QMutexLocker locker(&mutex);
    if (v != m_watching) {
        m_watching = v;
        if (!m_watching)
            delete std::exchange(m_watcher, nullptr);
    }
#else
    Q_UNUSED(v);
#endif
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void QFileInfoGatherer::clear()
{
#if QT_CONFIG(filesystemwatcher)
    QMutexLocker locker(&mutex);
    unwatchPaths(watchedFiles());
    unwatchPaths(watchedDirectories());
#endif
}

/*
    Remove a \a path from the watcher

    \sa listed()
*/
void QFileInfoGatherer::removePath(const QString &path)
{
#if QT_CONFIG(filesystemwatcher)
    QMutexLocker locker(&mutex);
    unwatchPaths(QStringList(path));
#else
    Q_UNUSED(path);
#endif
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void QFileInfoGatherer::list(const QString &directoryPath)
{
    fetchExtendedInformation(directoryPath, QStringList());
}

/*
    Until aborted wait to fetch a directory or files
*/
void QFileInfoGatherer::run()
{
    forever {
        // Disallow termination while we are holding a mutex or can be
        // woken up cleanly.
        setTerminationEnabled(false);
        QMutexLocker locker(&mutex);
        while (!isInterruptionRequested() && path.isEmpty())
            condition.wait(&mutex);
        if (isInterruptionRequested())
            return;
        const QString thisPath = std::as_const(path).front();
        path.pop_front();
        const QStringList thisList = std::as_const(files).front();
        files.pop_front();
        locker.unlock();

        // Some of the system APIs we call when gathering file infomration
        // might hang (e.g. waiting for network), so we explicitly allow
        // termination now.
        setTerminationEnabled(true);
        getFileInfos(thisPath, thisList);
    }
}

QExtendedInformation QFileInfoGatherer::getInfo(const QFileInfo &fileInfo) const
{
    QExtendedInformation info(fileInfo);
    if (m_iconProvider) {
        info.icon = m_iconProvider->icon(fileInfo);
        info.displayType = m_iconProvider->type(fileInfo);
    } else {
        info.displayType = QAbstractFileIconProviderPrivate::getFileType(fileInfo);
    }
#if QT_CONFIG(filesystemwatcher)
    // ### Not ready to listen all modifications by default
    static const bool watchFiles = qEnvironmentVariableIsSet("QT_FILESYSTEMMODEL_WATCH_FILES");
    if (watchFiles) {
        if (!fileInfo.exists() && !fileInfo.isSymLink()) {
            const_cast<QFileInfoGatherer *>(this)->
                unwatchPaths(QStringList(fileInfo.absoluteFilePath()));
        } else {
            const QString path = fileInfo.absoluteFilePath();
            if (!path.isEmpty() && fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()
                && !watchedFiles().contains(path)) {
                const_cast<QFileInfoGatherer *>(this)->watchPaths(QStringList(path));
            }
        }
    }
#endif // filesystemwatcher

#ifdef Q_OS_WIN
    if (m_resolveSymlinks && info.isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QFileInfo resolvedInfo(QFileInfo(fileInfo.symLinkTarget()).canonicalFilePath());
        if (resolvedInfo.exists()) {
            emit nameResolved(fileInfo.filePath(), resolvedInfo.fileName());
        }
    }
#endif
    return info;
}

/*
    Get specific file info's, batch the files so update when we have 100
    items and every 200ms after that
 */
void QFileInfoGatherer::getFileInfos(const QString &path, const QStringList &files)
{
    // List drives
    if (path.isEmpty()) {
#ifdef QT_BUILD_INTERNAL
        fetchedRoot.storeRelaxed(true);
#endif
        QList<std::pair<QString, QFileInfo>> updatedFiles;
        auto addToUpdatedFiles = [&updatedFiles](QFileInfo &&fileInfo) {
            fileInfo.stat();
            updatedFiles.emplace_back(std::pair{translateDriveName(fileInfo), fileInfo});
        };

        if (files.isEmpty()) {
            // QDir::drives() calls QFSFileEngine::drives() which creates the QFileInfoList on
            // the stack and return it, so this list is not shared, so no detaching.
            QFileInfoList infoList = QDir::drives();
            updatedFiles.reserve(infoList.size());
            for (auto rit = infoList.rbegin(), rend = infoList.rend(); rit != rend; ++rit)
                addToUpdatedFiles(std::move(*rit));
        } else {
            updatedFiles.reserve(files.size());
            for (auto rit = files.crbegin(), rend = files.crend(); rit != rend; ++rit)
                addToUpdatedFiles(QFileInfo(*rit));
        }
        emit updates(path, updatedFiles);
        return;
    }

    QElapsedTimer base;
    base.start();
    QFileInfo fileInfo;
    bool firstTime = true;
    QList<std::pair<QString, QFileInfo>> updatedFiles;
    QStringList filesToCheck = files;

    QStringList allFiles;
    if (files.isEmpty()) {
        // Use QDirListing::IteratorFlags when QFileSystemModel is
        // changed to use them too
        constexpr auto dirFilters = QDir::AllEntries | QDir::System | QDir::Hidden;
        for (const auto &dirEntry : QDirListing(path, {}, dirFilters.toInt())) {
            if (isInterruptionRequested())
                break;
            fileInfo = dirEntry.fileInfo();
            fileInfo.stat();
            allFiles.append(fileInfo.fileName());
            fetch(fileInfo, base, firstTime, updatedFiles, path);
        }
    }
    if (!allFiles.isEmpty())
        emit newListOfFiles(path, allFiles);

    QStringList::const_iterator filesIt = filesToCheck.constBegin();
    while (!isInterruptionRequested() && filesIt != filesToCheck.constEnd()) {
        fileInfo.setFile(path + QDir::separator() + *filesIt);
        ++filesIt;
        fileInfo.stat();
        fetch(fileInfo, base, firstTime, updatedFiles, path);
    }
    if (!updatedFiles.isEmpty())
        emit updates(path, updatedFiles);
    emit directoryLoaded(path);
}

void QFileInfoGatherer::fetch(const QFileInfo &fileInfo, QElapsedTimer &base, bool &firstTime,
                              QList<std::pair<QString, QFileInfo>> &updatedFiles, const QString &path)
{
    updatedFiles.emplace_back(std::pair(fileInfo.fileName(), fileInfo));
    QElapsedTimer current;
    current.start();
    if ((firstTime && updatedFiles.size() > 100) || base.msecsTo(current) > 1000) {
        emit updates(path, updatedFiles);
        updatedFiles.clear();
        base = current;
        firstTime = false;
    }
}

QT_END_NAMESPACE

#include "moc_qfileinfogatherer_p.cpp"
