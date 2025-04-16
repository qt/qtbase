// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "assetdownloader.h"

#include "tasking/concurrentcall.h"
#include "tasking/networkquery.h"
#include "tasking/tasktreerunner.h"

#include <QtCore/private/qzipreader_p.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>

using namespace Tasking;

QT_BEGIN_NAMESPACE

namespace Assets::Downloader {

struct DownloadableAssets
{
    QUrl remoteUrl;
    QList<QUrl> files;
};

class AssetDownloaderPrivate
{
public:
    AssetDownloaderPrivate(AssetDownloader *q) : m_q(q) {}
    AssetDownloader *m_q = nullptr;

    std::unique_ptr<QNetworkAccessManager> m_manager;
    std::unique_ptr<QTemporaryDir> m_temporaryDir;
    TaskTreeRunner m_taskTreeRunner;
    QString m_lastProgressText;
    QDir m_localDownloadDir;

    QString m_jsonFileName;
    QString m_zipFileName;
    QDir m_preferredLocalDownloadDir =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QUrl m_offlineAssetsFilePath;
    QUrl m_downloadBase;
    QStringList m_networkErrors;
    QStringList m_sslErrors;

    void setLocalDownloadDir(const QDir &dir)
    {
        if (m_localDownloadDir != dir) {
            m_localDownloadDir = dir;
            emit m_q->localDownloadDirChanged(QUrl::fromLocalFile(m_localDownloadDir.absolutePath()));
        }
    }
    void setProgress(int progressValue, int progressMaximum, const QString &progressText)
    {
        m_lastProgressText = progressText;
        emit m_q->progressChanged(progressValue, progressMaximum, progressText);
    }
    void updateProgress(int progressValue, int progressMaximum)
    {
        setProgress(progressValue, progressMaximum, m_lastProgressText);
    }
    void clearProgress(const QString &progressText)
    {
        setProgress(0, 0, progressText);
    }

    void setupDownload(NetworkQuery *query, const QString &progressText)
    {
        query->setNetworkAccessManager(m_manager.get());
        clearProgress(progressText);
        QObject::connect(query, &NetworkQuery::started, query, [this, query] {
            QNetworkReply *reply = query->reply();
            QObject::connect(reply, &QNetworkReply::downloadProgress,
                             query, [this](qint64 bytesReceived, qint64 totalBytes) {
                updateProgress((totalBytes > 0) ? 100.0 * bytesReceived / totalBytes : 0, 100);
            });
            QObject::connect(reply, &QNetworkReply::errorOccurred, query, [this, reply] {
                m_networkErrors << reply->errorString();
            });
#if QT_CONFIG(ssl)
            QObject::connect(reply, &QNetworkReply::sslErrors,
                             query, [this](const QList<QSslError> &sslErrors) {
                for (const QSslError &sslError : sslErrors)
                    m_sslErrors << sslError.errorString();
            });
#endif
        });
    }
};

static bool isWritableDir(const QDir &dir)
{
    if (dir.exists()) {
        QTemporaryFile file(dir.filePath(QString::fromLatin1("tmp")));
        return file.open();
    }
    return false;
}

static bool sameFileContent(const QFileInfo &first, const QFileInfo &second)
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

static bool createDirectory(const QDir &dir)
{
    if (dir.exists())
        return true;

    if (!createDirectory(dir.absoluteFilePath(QString::fromUtf8(".."))))
        return false;

    return dir.mkpath(QString::fromUtf8("."));
}

static bool canBeALocalBaseDir(const QDir &dir)
{
    if (dir.exists())
        return !dir.isEmpty() || isWritableDir(dir);
    return createDirectory(dir) && isWritableDir(dir);
}

static QDir baseLocalDir(const QDir &preferredLocalDir)
{
    if (canBeALocalBaseDir(preferredLocalDir))
        return preferredLocalDir;

    qWarning().noquote() << "AssetDownloader: Cannot set \"" << preferredLocalDir
                         << "\" as a local download directory!";
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

static QString pathFromUrl(const QUrl &url)
{
    if (url.isLocalFile())
        return url.toLocalFile();

    if (url.scheme() == u"qrc")
        return u":" + url.path();

    return url.toString();
}

static QList<QUrl> filterDownloadableAssets(const QList<QUrl> &assetFiles, const QDir &expectedDir)
{
    QList<QUrl> downloadList;
    std::copy_if(assetFiles.begin(), assetFiles.end(), std::back_inserter(downloadList),
                 [&](const QUrl &assetPath) {
        return !QFileInfo::exists(expectedDir.absoluteFilePath(assetPath.toString()));
    });
    return downloadList;
}

static bool allAssetsPresent(const QList<QUrl> &assetFiles, const QDir &expectedDir)
{
    return std::all_of(assetFiles.begin(), assetFiles.end(), [&](const QUrl &assetPath) {
        return QFileInfo::exists(expectedDir.absoluteFilePath(assetPath.toString()));
    });
}

AssetDownloader::AssetDownloader(QObject *parent)
    : QObject(parent)
    , d(new AssetDownloaderPrivate(this))
{}

AssetDownloader::~AssetDownloader() = default;

QUrl AssetDownloader::downloadBase() const
{
    return d->m_downloadBase;
}

void AssetDownloader::setDownloadBase(const QUrl &downloadBase)
{
    if (d->m_downloadBase != downloadBase) {
        d->m_downloadBase = downloadBase;
        emit downloadBaseChanged(d->m_downloadBase);
    }
}

QUrl AssetDownloader::preferredLocalDownloadDir() const
{
    return QUrl::fromLocalFile(d->m_preferredLocalDownloadDir.absolutePath());
}

void AssetDownloader::setPreferredLocalDownloadDir(const QUrl &localDir)
{
    if (localDir.scheme() == u"qrc") {
        qWarning() << "Cannot set a qrc as preferredLocalDownloadDir";
        return;
    }

    const QString path = pathFromUrl(localDir);
    if (d->m_preferredLocalDownloadDir != path) {
        d->m_preferredLocalDownloadDir.setPath(path);
        emit preferredLocalDownloadDirChanged(preferredLocalDownloadDir());
    }
}

QUrl AssetDownloader::offlineAssetsFilePath() const
{
    return d->m_offlineAssetsFilePath;
}

void AssetDownloader::setOfflineAssetsFilePath(const QUrl &offlineAssetsFilePath)
{
    if (d->m_offlineAssetsFilePath != offlineAssetsFilePath) {
        d->m_offlineAssetsFilePath = offlineAssetsFilePath;
        emit offlineAssetsFilePathChanged(d->m_offlineAssetsFilePath);
    }
}

QString AssetDownloader::jsonFileName() const
{
    return d->m_jsonFileName;
}

void AssetDownloader::setJsonFileName(const QString &jsonFileName)
{
    if (d->m_jsonFileName != jsonFileName) {
        d->m_jsonFileName = jsonFileName;
        emit jsonFileNameChanged(d->m_jsonFileName);
    }
}

QString AssetDownloader::zipFileName() const
{
    return d->m_zipFileName;
}

void AssetDownloader::setZipFileName(const QString &zipFileName)
{
    if (d->m_zipFileName != zipFileName) {
        d->m_zipFileName = zipFileName;
        emit zipFileNameChanged(d->m_zipFileName);
    }
}

QUrl AssetDownloader::localDownloadDir() const
{
    return QUrl::fromLocalFile(d->m_localDownloadDir.absolutePath());
}

QStringList AssetDownloader::networkErrors() const
{
    return d->m_networkErrors;
}

QStringList AssetDownloader::sslErrors() const
{
    return d->m_sslErrors;
}

static void precheckLocalFile(const QUrl &url)
{
    if (url.isEmpty())
        return;
    QFile file(pathFromUrl(url));
    if (!file.open(QIODevice::ReadOnly))
        qWarning() << "Cannot open local file" << url;
}

static void readAssetsFileContent(QPromise<DownloadableAssets> &promise, const QByteArray &content)
{
    const QJsonObject json = QJsonDocument::fromJson(content).object();
    const QJsonArray assetsArray = json[u"assets"].toArray();
    DownloadableAssets result;
    result.remoteUrl = json[u"url"].toString();
    for (const QJsonValue &asset : assetsArray) {
        if (promise.isCanceled())
            return;
        result.files.append(asset.toString());
    }

    if (result.files.isEmpty() || result.remoteUrl.isEmpty())
        promise.future().cancel();
    else
        promise.addResult(result);
}

static void unzip(QPromise<void> &promise, const QByteArray &content, const QDir &directory,
                  const QString &fileName)
{
    const QString zipFilePath = directory.absoluteFilePath(fileName);
    QFile zipFile(zipFilePath);
    if (!zipFile.open(QIODevice::WriteOnly)) {
        promise.future().cancel();
        return;
    }
    zipFile.write(content);
    zipFile.close();

    if (promise.isCanceled())
        return;

    QZipReader reader(zipFilePath);
    const bool extracted = reader.extractAll(directory.absolutePath());
    reader.close();
    if (extracted)
        QFile::remove(zipFilePath);
    else
        promise.future().cancel();
}

static void writeAsset(QPromise<void> &promise, const QByteArray &content, const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    QFile file(fileInfo.absoluteFilePath());
    if (!createDirectory(fileInfo.dir()) || !file.open(QFile::WriteOnly)) {
        promise.future().cancel();
        return;
    }

    if (promise.isCanceled())
        return;

    file.write(content);
    file.close();
}

static void copyAndCheck(QPromise<void> &promise, const QString &sourcePath, const QString &destPath)
{
    QFile sourceFile(sourcePath);
    QFile destFile(destPath);
    const QFileInfo sourceFileInfo(sourceFile.fileName());
    const QFileInfo destFileInfo(destFile.fileName());

    if (destFile.exists() && !destFile.remove()) {
        qWarning().noquote() << QString::fromLatin1("Unable to remove file \"%1\".")
                                        .arg(QFileInfo(destFile.fileName()).absoluteFilePath());
        promise.future().cancel();
        return;
    }

    if (!createDirectory(destFileInfo.absolutePath())) {
        qWarning().noquote() << QString::fromLatin1("Cannot create directory \"%1\".")
                                        .arg(destFileInfo.absolutePath());
        promise.future().cancel();
        return;
    }

    if (promise.isCanceled())
        return;

    if (!sourceFile.copy(destFile.fileName()) && !sameFileContent(sourceFileInfo, destFileInfo))
        promise.future().cancel();
}

void AssetDownloader::start()
{
    if (d->m_taskTreeRunner.isRunning())
        return;

    struct StorageData
    {
        QDir tempDir;
        QByteArray jsonContent;
        DownloadableAssets assets;
        QList<QUrl> assetsToDownload;
        QByteArray zipContent;
        int doneCount = 0;
    };

    const Storage<StorageData> storage;

    const auto onSetup = [this, storage] {
        if (!d->m_manager)
            d->m_manager = std::make_unique<QNetworkAccessManager>();
        if (!d->m_temporaryDir)
            d->m_temporaryDir = std::make_unique<QTemporaryDir>();
        if (!d->m_temporaryDir->isValid()) {
            qWarning() << "Cannot create a temporary directory.";
            return SetupResult::StopWithError;
        }
        storage->tempDir = d->m_temporaryDir->path();
        d->setLocalDownloadDir(baseLocalDir(d->m_preferredLocalDownloadDir));
        d->m_networkErrors.clear();
        d->m_sslErrors.clear();
        precheckLocalFile(resolvedUrl(d->m_offlineAssetsFilePath));
        return SetupResult::Continue;
    };

    const auto onJsonDownloadSetup = [this](NetworkQuery &query) {
        query.setRequest(QNetworkRequest(d->m_downloadBase.resolved(d->m_jsonFileName)));
        d->setupDownload(&query, tr("Downloading JSON file..."));
    };
    const auto onJsonDownloadDone = [this, storage](const NetworkQuery &query, DoneWith result) {
        if (result == DoneWith::Success) {
            storage->jsonContent = query.reply()->readAll();
            return DoneResult::Success;
        }
        qWarning() << "Cannot download" << d->m_downloadBase.resolved(d->m_jsonFileName)
                   << query.reply()->errorString();
        if (d->m_offlineAssetsFilePath.isEmpty()) {
            qWarning() << "Also there is no local file as a replacement";
            return DoneResult::Error;
        }

        QFile file(pathFromUrl(resolvedUrl(d->m_offlineAssetsFilePath)));
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Also failed to open" << d->m_offlineAssetsFilePath;
            return DoneResult::Error;
        }

        storage->jsonContent = file.readAll();
        return DoneResult::Success;
    };

    const auto onReadAssetsFileSetup = [storage](ConcurrentCall<DownloadableAssets> &async) {
        async.setConcurrentCallData(readAssetsFileContent, storage->jsonContent);
    };
    const auto onReadAssetsFileDone = [storage](const ConcurrentCall<DownloadableAssets> &async) {
        storage->assets = async.result();
        storage->assetsToDownload = storage->assets.files;
    };

    const auto onSkipIfAllAssetsPresent = [this, storage] {
        return allAssetsPresent(storage->assets.files, d->m_localDownloadDir)
                ? SetupResult::StopWithSuccess : SetupResult::Continue;
    };

    const auto onZipDownloadSetup = [this, storage](NetworkQuery &query) {
        if (d->m_zipFileName.isEmpty())
            return SetupResult::StopWithSuccess;

        query.setRequest(QNetworkRequest(d->m_downloadBase.resolved(d->m_zipFileName)));
        d->setupDownload(&query, tr("Downloading zip file..."));
        return SetupResult::Continue;
    };
    const auto onZipDownloadDone = [storage](const NetworkQuery &query, DoneWith result) {
        if (result == DoneWith::Success)
            storage->zipContent = query.reply()->readAll();
        return DoneResult::Success; // Ignore zip download failure
    };

    const auto onUnzipSetup = [this, storage](ConcurrentCall<void> &async) {
        if (storage->zipContent.isEmpty())
            return SetupResult::StopWithSuccess;

        async.setConcurrentCallData(unzip, storage->zipContent, storage->tempDir, d->m_zipFileName);
        d->clearProgress(tr("Unzipping..."));
        return SetupResult::Continue;
    };
    const auto onUnzipDone = [storage](DoneWith result) {
        if (result == DoneWith::Success) {
            // Avoid downloading assets that are present in unzipped tree
            StorageData &storageData = *storage;
            storageData.assetsToDownload =
                    filterDownloadableAssets(storageData.assets.files, storageData.tempDir);
        } else {
            qWarning() << "ZipFile failed";
        }
        return DoneResult::Success; // Ignore unzip failure
    };

    const LoopUntil downloadIterator([storage](int iteration) {
        return iteration < storage->assetsToDownload.count();
    });

    const Storage<QByteArray> assetStorage;

    const auto onAssetsDownloadGroupSetup = [this, storage] {
        d->setProgress(0, storage->assetsToDownload.size(), tr("Downloading assets..."));
    };

    const auto onAssetDownloadSetup = [this, storage, downloadIterator](NetworkQuery &query) {
        query.setNetworkAccessManager(d->m_manager.get());
        query.setRequest(QNetworkRequest(storage->assets.remoteUrl.resolved(
            storage->assetsToDownload.at(downloadIterator.iteration()))));
     };
    const auto onAssetDownloadDone = [assetStorage](const NetworkQuery &query, DoneWith result) {
        if (result == DoneWith::Success)
            *assetStorage = query.reply()->readAll();
    };

    const auto onAssetWriteSetup = [storage, downloadIterator, assetStorage](
                                           ConcurrentCall<void> &async) {
        const QString filePath = storage->tempDir.absoluteFilePath(
            storage->assetsToDownload.at(downloadIterator.iteration()).toString());
        async.setConcurrentCallData(writeAsset, *assetStorage, filePath);
    };
    const auto onAssetWriteDone = [this, storage](DoneWith result) {
        if (result != DoneWith::Success) {
            qWarning() << "Asset write failed";
            return;
        }
        StorageData &storageData = *storage;
        ++storageData.doneCount;
        d->updateProgress(storageData.doneCount, storageData.assetsToDownload.size());
    };

    const LoopUntil copyIterator([storage](int iteration) {
        return iteration < storage->assets.files.count();
    });

    const auto onAssetsCopyGroupSetup = [this, storage] {
        storage->doneCount = 0;
        d->setProgress(0, storage->assets.files.size(), tr("Copying assets..."));
    };

    const auto onAssetCopySetup = [this, storage, copyIterator](ConcurrentCall<void> &async) {
        const QString fileName = storage->assets.files.at(copyIterator.iteration()).toString();
        const QString sourcePath = storage->tempDir.absoluteFilePath(fileName);
        const QString destPath = d->m_localDownloadDir.absoluteFilePath(fileName);
        async.setConcurrentCallData(copyAndCheck, sourcePath, destPath);
    };
    const auto onAssetCopyDone = [this, storage] {
        StorageData &storageData = *storage;
        ++storageData.doneCount;
        d->updateProgress(storageData.doneCount, storageData.assets.files.size());
    };

    const auto onAssetsCopyGroupDone = [this, storage](DoneWith result) {
        if (result != DoneWith::Success) {
            d->setLocalDownloadDir(storage->tempDir);
            qWarning() << "Asset copy failed";
            return;
        }
        d->m_temporaryDir.reset();
    };

    const Group recipe {
        storage,
        onGroupSetup(onSetup),
        NetworkQueryTask(onJsonDownloadSetup, onJsonDownloadDone),
        ConcurrentCallTask<DownloadableAssets>(onReadAssetsFileSetup, onReadAssetsFileDone, CallDoneIf::Success),
        Group {
            onGroupSetup(onSkipIfAllAssetsPresent),
            NetworkQueryTask(onZipDownloadSetup, onZipDownloadDone),
            ConcurrentCallTask<void>(onUnzipSetup, onUnzipDone),
            For (downloadIterator) >> Do {
                parallelIdealThreadCountLimit,
                onGroupSetup(onAssetsDownloadGroupSetup),
                Group {
                    assetStorage,
                    NetworkQueryTask(onAssetDownloadSetup, onAssetDownloadDone),
                    ConcurrentCallTask<void>(onAssetWriteSetup, onAssetWriteDone)
                }
            },
            For (copyIterator) >> Do {
                parallelIdealThreadCountLimit,
                onGroupSetup(onAssetsCopyGroupSetup),
                ConcurrentCallTask<void>(onAssetCopySetup, onAssetCopyDone, CallDoneIf::Success),
                onGroupDone(onAssetsCopyGroupDone)
            }
        }
    };
    d->m_taskTreeRunner.start(recipe, [this](TaskTree *) { emit started(); },
            [this](DoneWith result) { emit finished(result == DoneWith::Success); });
}

QUrl AssetDownloader::resolvedUrl(const QUrl &url) const
{
    return url;
}

} // namespace Assets::Downloader

QT_END_NAMESPACE
