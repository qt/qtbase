// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ASSETDOWNLOADER_H
#define ASSETDOWNLOADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

namespace QtExamples {

class AssetDownloaderPrivate;
class AssetDownloader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
        QUrl downloadBase
        READ downloadBase
        WRITE setDownloadBase
        NOTIFY downloadBaseChanged)

    Q_PROPERTY(
        int completedDownloadsCount
        READ completedDownloadsCount
        NOTIFY completedDownloadsCountChanged)

    Q_PROPERTY(
        int allDownloadsCount
        READ allDownloadsCount
        NOTIFY allDownloadsCountChanged)

    Q_PROPERTY(
        double progress
        READ progress
        NOTIFY progressChanged)

    Q_PROPERTY(
        QUrl preferredLocalDownloadDir
        READ preferredLocalDownloadDir
        WRITE setPreferredLocalDownloadDir
        NOTIFY preferredLocalDownloadDirChanged)

    Q_PROPERTY(
        QUrl localDownloadDir
        READ localDownloadDir
        NOTIFY localDownloadDirChanged)

    Q_PROPERTY(
        QUrl offlineAssetsFilePath
        READ offlineAssetsFilePath
        WRITE setOfflineAssetsFilePath
        NOTIFY offlineAssetsFilePathChanged)

    Q_PROPERTY(
        QString jsonFileName
        READ jsonFileName
        WRITE setJsonFileName
        NOTIFY jsonFileNameChanged)

    Q_PROPERTY(
        QString zipFileName
        READ zipFileName
        WRITE setZipFileName
        NOTIFY zipFileNameChanged)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    enum class State {
        NotDownloadedState,
        DownloadingZipState,
        ExtractingZipState,
        DownloadingFilesState,
        MovingFilesState,
        DownloadedState,
        ErrorState
    };
    Q_ENUM(State)

    AssetDownloader(QObject *parent = nullptr);
    ~AssetDownloader();

    QUrl downloadBase() const;
    void setDownloadBase(const QUrl &downloadBase);

    int allDownloadsCount() const;
    int completedDownloadsCount() const;
    QUrl localDownloadDir() const;
    double progress() const;

    QUrl preferredLocalDownloadDir() const;
    void setPreferredLocalDownloadDir(const QUrl &localDir);

    QUrl offlineAssetsFilePath() const;
    void setOfflineAssetsFilePath(const QUrl &offlineAssetsFilePath);

    QString jsonFileName() const;
    void setJsonFileName(const QString &jsonFileName);

    QString zipFileName() const;
    void setZipFileName(const QString &zipFileName);

    State state() const;

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void setProgressPercent(int);

Q_SIGNALS:
    void stateChanged(State);
    void downloadBaseChanged(const QUrl &);
    void allDownloadsCountChanged(int count);
    void downloadStarted();
    void completedDownloadsCountChanged(int count);
    void downloadFinished();
    void downloadingFileChanged(const QUrl &url);
    void preferredLocalDownloadDirChanged(const QUrl &url);
    void localDownloadDirChanged(const QUrl &url);
    void progressChanged(double progress);
    void offlineAssetsFilePathChanged(const QUrl &);
    void jsonFileNameChanged(const QString &);
    void zipFileNameChanged(const QString &);

private:
    Q_DECLARE_PRIVATE(AssetDownloader)
    QScopedPointer<AssetDownloaderPrivate> d_ptr;
};

} // namespace QtExamples

QT_END_NAMESPACE

#endif // ASSETDOWNLOADER_H
