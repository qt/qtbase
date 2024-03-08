// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "assetdownloader.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QtAssert>
#include <QtCore/private/qzipreader_p.h>

#include <QCoreApplication>
#include <QFile>
#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLatin1StringView>
#include <QNetworkReply>
#include <QNetworkRequest>

using QtExamples::AssetDownloader;
using QtExamples::AssetDownloaderPrivate;

namespace {

enum class UnzipRule { DeleteArchive, KeepArchive };

struct DownloadableAssets
{
    QUrl remoteUrl;
    QList<QUrl> files;
};

class Exception : public QException
{
public:
    Exception(const QString &errorString)
        : m_errorString(errorString)
    {}

    QString message() const { return m_errorString; }

private:
    QString m_errorString;
};

class NetworkException : public Exception
{
public:
    NetworkException(QNetworkReply *reply)
        : Exception(reply->errorString())
    {}

    void raise() const override { throw *this; }
    NetworkException *clone() const override { return new NetworkException(*this); }
};

class FileException : public Exception
{
public:
    FileException(QFile *file)
        : Exception(QString::fromUtf8("%1 \"%2\"").arg(file->errorString(), file->fileName()))
    {}

    void raise() const override { throw *this; }
    FileException *clone() const override { return new FileException(*this); }
};

class FileRemoveException : public Exception
{
public:
    FileRemoveException(QFile *file)
        : Exception(QString::fromLatin1("Unable to remove file \"%1\"")
                        .arg(QFileInfo(file->fileName()).absoluteFilePath()))
    {}

    void raise() const override { throw *this; }
    FileRemoveException *clone() const override { return new FileRemoveException(*this); }
};

class FileCopyException : public Exception
{
public:
    FileCopyException(const QString &src, const QString &dst)
        : Exception(QString::fromLatin1("Unable to copy file \"%1\" to \"%2\"")
                        .arg(QFileInfo(src).absoluteFilePath(), QFileInfo(dst).absoluteFilePath()))
    {}

    void raise() const override { throw *this; }
    FileCopyException *clone() const override { return new FileCopyException(*this); }
};

class DirException : public Exception
{
public:
    DirException(const QDir &dir)
        : Exception(QString::fromLatin1("Cannot create directory %1").arg(dir.absolutePath()))
    {}

    void raise() const override { throw *this; }
    DirException *clone() const override { return new DirException(*this); }
};

class AssetFileException : public Exception
{
public:
    AssetFileException()
        : Exception(QString::fromUtf8("Asset file is broken."))
    {}
    void raise() const override { throw *this; }
    AssetFileException *clone() const override { return new AssetFileException(*this); }
};

bool isWritableDir(const QDir &dir)
{
    if (dir.exists()) {
        QTemporaryFile file(dir.filePath(QString::fromLatin1("tmp")));
        return file.open();
    }
    return false;
}

bool sameFileContent(const QFileInfo &first, const QFileInfo &second)
{
    if (first.exists() ^ second.exists())
        return false;

    if (first.size() != second.size())
        return false;

    QFile firstFile(first.absoluteFilePath());
    QFile secondFile(second.absoluteFilePath());

    if (firstFile.open(QFile::ReadOnly) && secondFile.open(QFile::ReadOnly)) {
        char char1;
        char char2;
        int readBytes1 = 0;
        int readBytes2 = 0;
        while (!firstFile.atEnd()) {
            readBytes1 = firstFile.read(&char1, 1);
            readBytes2 = secondFile.read(&char2, 1);
            if (readBytes1 != readBytes2 || readBytes1 != 1)
                return false;
            if (char1 != char2)
                return false;
        }
        return true;
    }

    return false;
}

bool createDirectory(const QDir &dir)
{
    if (dir.exists())
        return true;

    if (!createDirectory(dir.absoluteFilePath(QString::fromUtf8(".."))))
        return false;

    return dir.mkpath(QString::fromUtf8("."));
}

bool canBeALocalBaseDir(QDir &dir)
{
    if (dir.exists()) {
        if (dir.isEmpty())
            return isWritableDir(dir);
        return true;
    } else {
        return createDirectory(dir) && isWritableDir(dir);
    }
    return false;
}

bool unzip(const QString &fileName, const QDir &directory, UnzipRule rule)
{
    QZipReader zipFile(fileName);
    const bool zipExtracted = zipFile.extractAll(directory.absolutePath());
    zipFile.close();
    if (zipExtracted && (rule == UnzipRule::DeleteArchive))
        QFile::remove(fileName);
    return zipExtracted;
}

void ensureDirOrThrow(const QDir &dir)
{
    if (createDirectory(dir))
        return;

    throw DirException(dir);
}

void ensureRemoveFileOrThrow(QFile &file)
{
    if (!file.exists() || file.remove())
        return;

    throw FileRemoveException(&file);
}

bool shouldDownloadEverything(const QList<QUrl> &assetFiles, const QDir &expectedDir)
{
    return std::any_of(assetFiles.begin(), assetFiles.end(), [&](const QUrl &imgPath) -> bool {
        const QString localPath = expectedDir.absoluteFilePath(imgPath.toString());
        return !QFileInfo::exists(localPath);
    });
}

QString pathFromUrl(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

DownloadableAssets readAssetsFile(const QByteArray &assetFileContent)
{
    DownloadableAssets result;
    QJsonObject json = QJsonDocument::fromJson(assetFileContent).object();

    const QJsonArray assetsArray = json[u"assets"].toArray();
    for (const QJsonValue &asset : assetsArray)
        result.files.append(asset.toString());

    result.remoteUrl = json[u"url"].toString();

    if (result.files.isEmpty() || result.remoteUrl.isEmpty())
        throw AssetFileException{};
    return result;
}

QList<QUrl> filterDownloadableAssets(const QList<QUrl> &assetFiles, const QDir &expectedDir)
{
    QList<QUrl> downloadList;
    std::copy_if(assetFiles.begin(),
                 assetFiles.end(),
                 std::back_inserter(downloadList),
                 [&](const QUrl &imgPath) {
                     const QString tempFilePath = expectedDir.absoluteFilePath(imgPath.toString());
                     return !QFileInfo::exists(tempFilePath);
                 });
    return downloadList;
}

} // namespace

class QtExamples::AssetDownloaderPrivate
{
    AssetDownloader *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(AssetDownloader)

public:
    using State = AssetDownloader::State;

    AssetDownloaderPrivate(AssetDownloader *parent);
    ~AssetDownloaderPrivate();

    void setupBase();
    void setBaseLocalDir(const QDir &dir);
    void moveAllAssets();
    void setAllDownloadsCount(int count);
    void setCompletedDownloadsCount(int count);
    void finalizeDownload();
    void setState(State state);

    QFuture<QByteArray> download(const QUrl &url);
    QFuture<QUrl> downloadToFile(const QUrl &url, const QUrl &destPath);
    QFuture<QByteArray> downloadOrRead(const QUrl &url, const QUrl &localFile);
    QFuture<DownloadableAssets> downloadAssets(const DownloadableAssets &assets);
    QFuture<DownloadableAssets> maybeReadZipFile(const DownloadableAssets &assets);

    State state = State::NotDownloadedState;
    int allDownloadsCount = 0;
    std::unique_ptr<QTemporaryDir> providedTempDir;
    QDir baseLocal;
    QDir tempDir;
    int completedDownloadsCount = 0;
    double progress = 0;
    bool downloadStarted = false;
    DownloadableAssets downloadableAssets;
    QNetworkAccessManager *networkManager = nullptr;
    QFutureWatcher<QByteArray> *downloadWatcher = nullptr;
    QFutureWatcher<QUrl> *fileDownloadWatcher = nullptr;

    QString jsonFileName;
    QString zipFileName;

    QDir preferredLocalDownloadDir;
    QUrl offlineAssetsFilePath;

    QUrl downloadBase = {QString::fromLatin1("https://download.qt.io/learning/examples/")};
};

AssetDownloaderPrivate::AssetDownloaderPrivate(AssetDownloader *parent)
    : q_ptr(parent)
    , networkManager(new QNetworkAccessManager(parent))
    , downloadWatcher(new QFutureWatcher<QByteArray>(parent))
    , fileDownloadWatcher(new QFutureWatcher<QUrl>(parent))
    , preferredLocalDownloadDir(
          QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
{
    QObject::connect(downloadWatcher,
                     &QFutureWatcherBase::progressValueChanged,
                     parent,
                     &AssetDownloader::setProgressPercent);

    QObject::connect(fileDownloadWatcher,
                     &QFutureWatcherBase::progressValueChanged,
                     parent,
                     &AssetDownloader::setProgressPercent);
}

AssetDownloaderPrivate::~AssetDownloaderPrivate()
{
    downloadWatcher->cancel();
    fileDownloadWatcher->cancel();
    downloadWatcher->waitForFinished();
    fileDownloadWatcher->waitForFinished();
}

void AssetDownloaderPrivate::setupBase()
{
    providedTempDir.reset(new QTemporaryDir{});
    Q_ASSERT_X(providedTempDir->isValid(),
               "AssetDownloader::setupBase()",
               "Cannot create a temporary directory!");

    if (canBeALocalBaseDir(preferredLocalDownloadDir)) {
        setBaseLocalDir(preferredLocalDownloadDir);
    } else {
        qWarning().noquote() << "AssetDownloader: Cannot set \"" << preferredLocalDownloadDir
                             << "\" as a local download directory!";
        setBaseLocalDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    }

    tempDir = providedTempDir->path();
}

void AssetDownloaderPrivate::setBaseLocalDir(const QDir &dir)
{
    Q_Q(AssetDownloader);
    if (baseLocal != dir) {
        baseLocal = dir;
        emit q->localDownloadDirChanged(QUrl::fromLocalFile(baseLocal.absolutePath()));
    }
}

void AssetDownloaderPrivate::moveAllAssets()
{
    if (tempDir != baseLocal && !tempDir.isEmpty()) {
        try {
            setState(State::MovingFilesState);
            QtConcurrent::blockingMap(downloadableAssets.files, [this](const QUrl &asset) {
                QFile srcFile(tempDir.absoluteFilePath(asset.toString()));
                QFile dstFile(baseLocal.absoluteFilePath(asset.toString()));
                QFileInfo srcFileInfo(srcFile.fileName());
                QFileInfo dstFileInfo(dstFile.fileName());

                ensureRemoveFileOrThrow(dstFile);
                ensureDirOrThrow(dstFileInfo.absolutePath());

                if (!srcFile.copy(dstFile.fileName())) {
                    if (!sameFileContent(srcFileInfo, dstFileInfo))
                        throw FileCopyException(srcFile.fileName(), dstFile.fileName());
                }
            });
        } catch (const Exception &exception) {
            qWarning() << exception.message();
            setBaseLocalDir(tempDir);
        } catch (const std::exception &exception) {
            qWarning() << exception.what();
            setBaseLocalDir(tempDir);
        }
    }
}

void AssetDownloaderPrivate::setAllDownloadsCount(int count)
{
    Q_Q(AssetDownloader);
    if (allDownloadsCount != count) {
        allDownloadsCount = count;
        emit q->allDownloadsCountChanged(allDownloadsCount);
    }
}

void AssetDownloaderPrivate::setCompletedDownloadsCount(int count)
{
    Q_Q(AssetDownloader);
    if (completedDownloadsCount != count) {
        completedDownloadsCount = count;
        emit q->completedDownloadsCountChanged(completedDownloadsCount);
    }
}

void AssetDownloaderPrivate::finalizeDownload()
{
    Q_Q(AssetDownloader);
    if (downloadStarted) {
        downloadStarted = false;
        emit q->downloadFinished();
    }
}

void AssetDownloaderPrivate::setState(State state)
{
    Q_Q(AssetDownloader);
    if (this->state != state) {
        this->state = state;
        emit q->stateChanged(this->state);
    }
}

QFuture<QByteArray> AssetDownloaderPrivate::download(const QUrl &url)
{
    Q_Q(AssetDownloader);
    Q_ASSERT_X(QThread::currentThread() != q->thread(),
               "AssetDownloader::download()",
               "Method called from wrong thread");

    QSharedPointer<QPromise<QByteArray>> promise(new QPromise<QByteArray>);
    QFuture<QByteArray> result = promise->future();
    promise->start();

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, promise]() {
        if (reply->error() == QNetworkReply::NoError)
            promise->addResult(reply->readAll());
        else
            promise->setException(NetworkException(reply));
        promise->finish();
        reply->deleteLater();
    });

    QObject::connect(reply,
                     &QNetworkReply::downloadProgress,
                     reply,
                     [promise, reply](qint64 bytesReceived, qint64 totalBytes) {
                         if (promise->isCanceled()) {
                             reply->abort();
                             return;
                         }
                         int progress = (totalBytes > 0) ? 100.0 * bytesReceived / totalBytes : 0;
                         promise->setProgressRange(0, 100);
                         promise->setProgressValue(progress);
                     });

    downloadWatcher->setFuture(result);
    emit q->downloadingFileChanged(url);

    return result;
}

QFuture<QUrl> AssetDownloaderPrivate::downloadToFile(const QUrl &url, const QUrl &destPath)
{
    Q_Q(AssetDownloader);
    Q_ASSERT_X(QThread::currentThread() != q->thread(),
               "AssetDownloader::download()",
               "Method called from wrong thread");

    QSharedPointer<QPromise<QUrl>> promise(new QPromise<QUrl>);
    QFuture<QUrl> result = promise->future();
    promise->start();

    QFileInfo fileInfo(pathFromUrl(destPath));
    createDirectory(fileInfo.absolutePath());

    QSharedPointer<QFile> file(new QFile(fileInfo.absoluteFilePath()));

    if (!file->open(QFile::WriteOnly)) {
        promise->setException(FileException(file.data()));
        promise->finish();
        return result;
    }

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url), file.data());
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, promise, destPath, file]() {
        if (reply->error() == QNetworkReply::NoError)
            promise->addResult(destPath);
        else
            promise->setException(NetworkException(reply));

        const qint64 bytesAvailable = reply->bytesAvailable();
        if (bytesAvailable)
            file->write(reply->read(bytesAvailable));
        file->close();
        promise->finish();
        reply->deleteLater();
    });

    QObject::connect(reply, &QNetworkReply::readyRead, reply, [promise, reply, file]() {
        if (promise->isCanceled()) {
            reply->abort();
            return;
        }
        const qint64 available = reply->bytesAvailable();
        file->write(reply->read(available));
    });

    QObject::connect(reply,
                     &QNetworkReply::downloadProgress,
                     reply,
                     [promise, reply](qint64 bytesReceived, qint64 totalBytes) {
                         if (promise->isCanceled()) {
                             reply->abort();
                             return;
                         }
                         int progress = (totalBytes > 0) ? 100.0 * bytesReceived / totalBytes : 0;
                         promise->setProgressRange(0, 100);
                         promise->setProgressValue(progress);
                     });

    fileDownloadWatcher->setFuture(result);
    emit q->downloadingFileChanged(url);

    return result;
}

QFuture<QByteArray> AssetDownloaderPrivate::downloadOrRead(const QUrl &url, const QUrl &localFile)
{
    Q_Q(AssetDownloader);
    Q_ASSERT_X(QThread::currentThread() != q->thread(),
               "AssetDownloader::downloadOrRead()",
               "Method called from wrong thread");

    QSharedPointer<QPromise<QByteArray>> promise(new QPromise<QByteArray>);
    QFuture<QByteArray> futureResult = promise->future();
    promise->start();

    // Precheck local file
    if (!localFile.isEmpty()) {
        QFile file(pathFromUrl(localFile));
        if (!file.open(QIODevice::ReadOnly))
            qWarning() << "Cannot open local file" << localFile;
    }

    download(url)
        .then([promise](const QByteArray &content) {
            if (promise->isCanceled())
                return;

            promise->addResult(content);
            promise->finish();
        })
        .onFailed([promise, url, localFile](const QException &downloadException) {
            QFile file(pathFromUrl(localFile));
            qWarning() << "Cannot download" << url;

            if (promise->isCanceled())
                return;

            if (localFile.isEmpty()) {
                qWarning() << "Also there is no local file as a replacement";
                promise->setException(downloadException);
            } else if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Also failed to open" << localFile;
                promise->setException(FileException(&file));
            } else {
                promise->addResult(file.readAll());
            }
            promise->finish();
        });

    return futureResult;
}

/*!
 * \internal
 * Schedules \a assets for downloading.
 * Returns a future representing the files which are not downloaded.
 * \note call this method on the AssetDownloader thread
 */
QFuture<DownloadableAssets> AssetDownloaderPrivate::downloadAssets(const DownloadableAssets &assets)
{
    Q_Q(AssetDownloader);

    Q_ASSERT_X(QThread::currentThread() != q->thread(),
               "AssetDownloader::downloadAssets()",
               "Method called from wrong thread");

    QSharedPointer<QPromise<DownloadableAssets>> promise(new QPromise<DownloadableAssets>);
    QFuture<DownloadableAssets> future = promise->future();

    promise->start();

    if (assets.files.isEmpty()) {
        promise->addResult(assets);
        promise->finish();
        return future;
    }

    setCompletedDownloadsCount(0);
    setAllDownloadsCount(assets.files.size());
    setState(State::DownloadingFilesState);

    QFuture<void> schedule = QtConcurrent::run([] {});

    for (const QUrl &assetFile : assets.files) {
        const QUrl url = assets.remoteUrl.resolved(assetFile);
        const QUrl dstFilePath = tempDir.absoluteFilePath(assetFile.toString());
        schedule = schedule
                       .then(q,
                             [this, url, dstFilePath] { return downloadToFile(url, dstFilePath); })
                       .unwrap()
                       .then(q,
                             [this, promise]([[maybe_unused]] const QUrl &url) {
                                 if (promise->isCanceled())
                                     return;
                                 setCompletedDownloadsCount(completedDownloadsCount + 1);
                             })
                       .onFailed([](const Exception &exception) {
                           qWarning() << "Download failed" << exception.message();
                       });
        ;
    }
    schedule = schedule
                   .then([assets, promise, tempDir = tempDir]() {
                       if (promise->isCanceled())
                           return;

                       DownloadableAssets result = assets;
                       result.files = filterDownloadableAssets(result.files, tempDir);
                       promise->addResult(result);
                       promise->finish();
                   })
                   .onFailed([promise](const QException &exception) {
                       promise->setException(exception);
                       promise->finish();
                   });
    return future;
}

QFuture<DownloadableAssets> AssetDownloaderPrivate::maybeReadZipFile(const DownloadableAssets &assets)
{
    Q_Q(AssetDownloader);
    Q_ASSERT_X(QThread::currentThread() != q->thread(),
               "AssetDownloader::maybeReadZipFile()",
               "Method called from wrong thread");

    QSharedPointer<QPromise<DownloadableAssets>> promise(new QPromise<DownloadableAssets>);
    QFuture<DownloadableAssets> future = promise->future();

    promise->start();

    if (assets.files.isEmpty() || zipFileName.isEmpty()) {
        promise->addResult(assets);
        promise->finish();
        return future;
    }

    setState(State::DownloadingZipState);

    const QUrl zipUrl = downloadBase.resolved(zipFileName);
    const QUrl dstUrl = tempDir.filePath(zipFileName);
    downloadToFile(zipUrl, dstUrl)
        .then(q,
              [this, promise](const QUrl &downloadedFile) -> QUrl {
                  if (promise->isCanceled())
                      return {};

                  setState(State::ExtractingZipState);
                  return downloadedFile;
              })
        .then(QtFuture::Launch::Async,
              [promise, zipFileName = zipFileName, tempDir = tempDir](const QUrl &downloadedFile) {
                  if (promise->isCanceled())
                      return;

                  unzip(pathFromUrl(downloadedFile), tempDir, UnzipRule::KeepArchive);
              })
        .then([promise, assets, tempDir = tempDir]() {
            if (promise->isCanceled())
                return;

            DownloadableAssets result = assets;
            result.files = filterDownloadableAssets(result.files, tempDir);
            promise->addResult(result);
            promise->finish();
        })
        .onFailed([promise, assets](const Exception &exception) {
            qWarning() << "ZipFile failed" << exception.message();
            promise->addResult(assets);
            promise->finish();
        });

    return future;
}

AssetDownloader::AssetDownloader(QObject *parent)
    : QObject(parent)
    , d_ptr(new AssetDownloaderPrivate(this))
{}

AssetDownloader::~AssetDownloader() = default;

QUrl AssetDownloader::downloadBase() const
{
    Q_D(const AssetDownloader);
    return d->downloadBase;
}

void AssetDownloader::setDownloadBase(const QUrl &downloadBase)
{
    Q_D(AssetDownloader);
    if (d->downloadBase != downloadBase) {
        d->downloadBase = downloadBase;
        emit downloadBaseChanged(d->downloadBase);
    }
}

int AssetDownloader::allDownloadsCount() const
{
    Q_D(const AssetDownloader);
    return d->allDownloadsCount;
}

int AssetDownloader::completedDownloadsCount() const
{
    Q_D(const AssetDownloader);
    return d->completedDownloadsCount;
}

QUrl AssetDownloader::localDownloadDir() const
{
    Q_D(const AssetDownloader);
    return QUrl::fromLocalFile(d->baseLocal.absolutePath());
}

double AssetDownloader::progress() const
{
    Q_D(const AssetDownloader);
    return d->progress;
}

QUrl AssetDownloader::preferredLocalDownloadDir() const
{
    Q_D(const AssetDownloader);
    return QUrl::fromLocalFile(d->preferredLocalDownloadDir.absolutePath());
}

void AssetDownloader::setPreferredLocalDownloadDir(const QUrl &localDir)
{
    Q_D(AssetDownloader);

    if (!localDir.isLocalFile())
        qWarning() << "preferredLocalDownloadDir Should be a local directory";

    const QString path = pathFromUrl(localDir);
    if (d->preferredLocalDownloadDir != path) {
        d->preferredLocalDownloadDir.setPath(path);
        emit preferredLocalDownloadDirChanged(preferredLocalDownloadDir());
    }
}

QUrl AssetDownloader::offlineAssetsFilePath() const
{
    Q_D(const AssetDownloader);
    return d->offlineAssetsFilePath;
}

void AssetDownloader::setOfflineAssetsFilePath(const QUrl &offlineAssetsFilePath)
{
    Q_D(AssetDownloader);
    if (d->offlineAssetsFilePath != offlineAssetsFilePath) {
        d->offlineAssetsFilePath = offlineAssetsFilePath;
        emit offlineAssetsFilePathChanged(d->offlineAssetsFilePath);
    }
}

QString AssetDownloader::jsonFileName() const
{
    Q_D(const AssetDownloader);
    return d->jsonFileName;
}

void AssetDownloader::setJsonFileName(const QString &jsonFileName)
{
    Q_D(AssetDownloader);
    if (d->jsonFileName != jsonFileName) {
        d->jsonFileName = jsonFileName;
        emit jsonFileNameChanged(d->jsonFileName);
    }
}

QString AssetDownloader::zipFileName() const
{
    Q_D(const AssetDownloader);
    return d->zipFileName;
}

void AssetDownloader::setZipFileName(const QString &zipFileName)
{
    Q_D(AssetDownloader);
    if (d->zipFileName != zipFileName) {
        d->zipFileName = zipFileName;
        emit zipFileNameChanged(d->zipFileName);
    }
}

AssetDownloader::State AssetDownloader::state() const
{
    Q_D(const AssetDownloader);
    return d->state;
}

void AssetDownloader::start()
{
    Q_D(AssetDownloader);
    if (d->downloadStarted)
        return;

    d->setState(State::NotDownloadedState);
    d->setupBase();

    d->downloadOrRead(d->downloadBase.resolved(d->jsonFileName), d->offlineAssetsFilePath)
        .then(QtFuture::Launch::Async, &readAssetsFile)
        .then(this,
              [this, d](const DownloadableAssets &assets) {
                  d->downloadStarted = true;
                  d->downloadableAssets = assets;
                  emit downloadStarted();
                  return assets;
              })
        .then(QtFuture::Launch::Async,
              [localDir = d->baseLocal](DownloadableAssets assets) -> DownloadableAssets {
                  if (shouldDownloadEverything(assets.files, localDir))
                      return assets;
                  return {};
              })
        .then(this, [d](const DownloadableAssets &assets) { return d->maybeReadZipFile(assets); })
        .unwrap()
        .then(this, [d](const DownloadableAssets &assets) { return d->downloadAssets(assets); })
        .unwrap()
        .then(this, [d](const DownloadableAssets &) { return d->moveAllAssets(); })
        .then(this,
              [d]() {
                  d->setState(State::DownloadedState);
                  d->finalizeDownload();
              })
        .onFailed(this, [d](const Exception &exception) {
            qWarning() << "Exception: " << exception.message();
            d->setState(State::ErrorState);
            d->finalizeDownload();
        });
    ;
}

void AssetDownloader::setProgressPercent(int progress)
{
    Q_D(AssetDownloader);
    const double progressInRange = 0.01 * progress;
    if (d->progress != progressInRange) {
        d->progress = progressInRange;
        emit progressChanged(d->progress);
    }
}
