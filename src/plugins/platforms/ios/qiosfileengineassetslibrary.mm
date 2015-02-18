/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosfileengineassetslibrary.h"

#import <UIKit/UIKit.h>
#import <AssetsLibrary/AssetsLibrary.h>

#include <QtCore/QTimer>
#include <QtCore/private/qcoreapplication_p.h>

class QIOSAssetData : public QObject
{
public:
    QIOSAssetData(const QString &assetUrl, QIOSFileEngineAssetsLibrary *engine)
        : m_asset(0)
        , m_assetUrl(assetUrl)
        , m_assetLibrary(0)
    {
        switch ([ALAssetsLibrary authorizationStatus]) {
        case ALAuthorizationStatusRestricted:
        case ALAuthorizationStatusDenied:
            engine->setError(QFile::PermissionsError, QLatin1String("Unauthorized access"));
            return;
        case ALAuthorizationStatusNotDetermined:
            if (!static_cast<QCoreApplicationPrivate *>(QObjectPrivate::get(qApp))->in_exec) {
                // Since authorization status has not been determined, the user will be asked
                // to authorize the app. But since main has not finished, the dialog will be held
                // back until the launch completes. To avoid a dead-lock below, we start an event
                // loop to complete the launch.
                QEventLoop loop;
                QTimer::singleShot(1, &loop, &QEventLoop::quit);
                loop.exec();
            }
            break;
        default:
            if (g_currentAssetData) {
                // It's a common pattern that QFiles pointing to the same path are created and destroyed
                // several times during a single event loop cycle. To avoid loading the same asset
                // over and over, we check if the last loaded asset has not been destroyed yet, and try to
                // reuse its data. Since QFile is (mostly) reentrant, we need to protect m_currentAssetData
                // from being modified by several threads at the same time.
                QMutexLocker lock(&g_mutex);
                if (g_currentAssetData && g_currentAssetData->m_assetUrl == assetUrl) {
                    m_assetLibrary = [g_currentAssetData->m_assetLibrary retain];
                    m_asset = [g_currentAssetData->m_asset retain];
                    return;
                }
            }
        }

        // We can only load images from the asset library async. And this might take time, since it
        // involves showing the authorization dialog. But the QFile API is synchronuous, so we need to
        // wait until we have access to the data. [ALAssetLibrary assetForUrl:] will shedule a block on
        // the current thread. But instead of spinning the event loop to force the block to execute, we
        // wrap the call inside a synchronuous dispatch queue so that it executes on another thread.
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            NSURL *url = [NSURL URLWithString:assetUrl.toNSString()];
            m_assetLibrary = [[ALAssetsLibrary alloc] init];
            [m_assetLibrary assetForURL:url resultBlock:^(ALAsset *asset) {
                m_asset = [asset retain];
                dispatch_semaphore_signal(semaphore);
            } failureBlock:^(NSError *error) {
                engine->setError(QFile::OpenError, QString::fromNSString(error.localizedDescription));
                dispatch_semaphore_signal(semaphore);
            }];
        });

        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        dispatch_release(semaphore);

        QMutexLocker lock(&g_mutex);
        g_currentAssetData = this;
    }

    ~QIOSAssetData()
    {
        QMutexLocker lock(&g_mutex);
        [m_assetLibrary release];
        [m_asset release];
        if (this == g_currentAssetData)
            g_currentAssetData = 0;
    }

    ALAsset *m_asset;

private:
    QString m_assetUrl;
    ALAssetsLibrary *m_assetLibrary;

    static QBasicMutex g_mutex;
    static QPointer<QIOSAssetData> g_currentAssetData;
};

QBasicMutex QIOSAssetData::g_mutex;
QPointer<QIOSAssetData> QIOSAssetData::g_currentAssetData = 0;

// -------------------------------------------------------------------------

QIOSFileEngineAssetsLibrary::QIOSFileEngineAssetsLibrary(const QString &fileName)
    : m_fileName(fileName)
    , m_offset(0)
    , m_data(0)
{
}

QIOSFileEngineAssetsLibrary::~QIOSFileEngineAssetsLibrary()
{
    close();
}

ALAsset *QIOSFileEngineAssetsLibrary::loadAsset() const
{
    if (!m_data) {
        // QUrl::fromLocalFile() will remove double slashes. Since the asset url is passed around as a file
        // name in the app (and converted to/from a file url, e.g in QFileDialog), we need to check if we still
        // have two leading slashes after the scheme, and restore the second slash if not.
        QString assetUrl = m_fileName;
        const int index = 16; // "assets-library://"
        if (assetUrl[index] != QLatin1Char('/'))
            assetUrl.insert(index, '/');

        m_data = new QIOSAssetData(assetUrl, const_cast<QIOSFileEngineAssetsLibrary *>(this));
    }

    return m_data->m_asset;
}

bool QIOSFileEngineAssetsLibrary::open(QIODevice::OpenMode openMode)
{
    if (openMode & (QIODevice::WriteOnly | QIODevice::Text))
        return false;
    return loadAsset();
}

bool QIOSFileEngineAssetsLibrary::close()
{
    if (m_data) {
        // Delete later, so that we can reuse the asset if a QFile is
        // opened with the same path during the same event loop cycle.
        m_data->deleteLater();
        m_data = 0;
    }
    return true;
}

QAbstractFileEngine::FileFlags QIOSFileEngineAssetsLibrary::fileFlags(QAbstractFileEngine::FileFlags type) const
{
    QAbstractFileEngine::FileFlags flags = 0;
    if (!loadAsset())
        return flags;

    if (type & FlagsMask)
        flags |= ExistsFlag;
    if (type & PermsMask)
        flags |= ReadOwnerPerm | ReadUserPerm | ReadGroupPerm | ReadOtherPerm;
    if (type & TypesMask)
        flags |= FileType;

    return flags;
}

qint64 QIOSFileEngineAssetsLibrary::size() const
{
    if (ALAsset *asset = loadAsset())
        return [[asset defaultRepresentation] size];
    return 0;
}

qint64 QIOSFileEngineAssetsLibrary::read(char *data, qint64 maxlen)
{
    ALAsset *asset = loadAsset();
    if (!asset)
        return -1;

    qint64 bytesRead = qMin(maxlen, size() - m_offset);
    if (!bytesRead)
        return 0;

    NSError *error = 0;
    [[asset defaultRepresentation] getBytes:(uint8_t *)data fromOffset:m_offset length:bytesRead error:&error];

    if (error) {
        setError(QFile::ReadError, QString::fromNSString(error.localizedDescription));
        return -1;
    }

    m_offset += bytesRead;
    return bytesRead;
}

qint64 QIOSFileEngineAssetsLibrary::pos() const
{
    return m_offset;
}

bool QIOSFileEngineAssetsLibrary::seek(qint64 pos)
{
    if (pos >= size())
        return false;
    m_offset = pos;
    return true;
}

QString QIOSFileEngineAssetsLibrary::fileName(FileName file) const
{
    Q_UNUSED(file);
    return m_fileName;
}

void QIOSFileEngineAssetsLibrary::setFileName(const QString &file)
{
    if (m_data)
        close();
    m_fileName = file;
}

QStringList QIOSFileEngineAssetsLibrary::entryList(QDir::Filters filters, const QStringList &filterNames) const
{
    Q_UNUSED(filters);
    Q_UNUSED(filterNames);
    return QStringList();
}
