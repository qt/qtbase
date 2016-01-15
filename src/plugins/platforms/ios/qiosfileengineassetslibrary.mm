/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qiosfileengineassetslibrary.h"

#import <UIKit/UIKit.h>
#import <AssetsLibrary/AssetsLibrary.h>

#include <QtCore/QTimer>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/qurl.h>
#include <QtCore/qset.h>
#include <QtCore/qthreadstorage.h>

static QThreadStorage<QString> g_iteratorCurrentUrl;
static QThreadStorage<QPointer<QIOSAssetData> > g_assetDataCache;

static const int kBufferSize = 10;
static ALAsset *kNoAsset = 0;

static bool ensureAuthorizationDialogNotBlocked()
{
    if ([ALAssetsLibrary authorizationStatus] != ALAuthorizationStatusNotDetermined)
        return true;

    if (static_cast<QCoreApplicationPrivate *>(QObjectPrivate::get(qApp))->in_exec)
        return true;

    if ([NSThread isMainThread]) {
        // The dialog is about to show, but since main has not finished, the dialog will be held
        // back until the launch completes. This is problematic since we cannot successfully return
        // back to the caller before the asset is ready, which also includes showing the dialog. To
        // work around this, we create an event loop to that will complete the launch (return from the
        // applicationDidFinishLaunching callback). But this will only work if we're on the main thread.
        QEventLoop loop;
        QTimer::singleShot(1, &loop, &QEventLoop::quit);
        loop.exec();
    } else {
        NSLog(@"QIOSFileEngine: unable to show assets authorization dialog from non-gui thread before QApplication is executing.");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------

class QIOSAssetEnumerator
{
public:
    QIOSAssetEnumerator(ALAssetsLibrary *assetsLibrary, ALAssetsGroupType type)
        : m_semWriteAsset(dispatch_semaphore_create(kBufferSize))
        , m_semReadAsset(dispatch_semaphore_create(0))
        , m_stop(false)
        , m_assetsLibrary([assetsLibrary retain])
        , m_type(type)
        , m_buffer(QVector<ALAsset *>(kBufferSize))
        , m_readIndex(0)
        , m_writeIndex(0)
        , m_nextAssetReady(false)
    {
        if (!ensureAuthorizationDialogNotBlocked())
            writeAsset(kNoAsset);
        else
            startEnumerate();
    }

    ~QIOSAssetEnumerator()
    {
        m_stop = true;

        // Flush and autorelease remaining assets in the buffer
        while (hasNext())
            next();

        // Documentation states that we need to balance out calls to 'wait'
        // and 'signal'. Since the enumeration function always will be one 'wait'
        // ahead, we need to signal m_semProceedToNextAsset one last time.
        dispatch_semaphore_signal(m_semWriteAsset);
        dispatch_release(m_semReadAsset);
        dispatch_release(m_semWriteAsset);

        [m_assetsLibrary autorelease];
    }

    bool hasNext()
    {
        if (!m_nextAssetReady) {
            dispatch_semaphore_wait(m_semReadAsset, DISPATCH_TIME_FOREVER);
            m_nextAssetReady = true;
        }
        return m_buffer[m_readIndex] != kNoAsset;
    }

    ALAsset *next()
    {
        Q_ASSERT(m_nextAssetReady);
        Q_ASSERT(m_buffer[m_readIndex]);

        ALAsset *asset = [m_buffer[m_readIndex] autorelease];
        dispatch_semaphore_signal(m_semWriteAsset);

        m_readIndex = (m_readIndex + 1) % kBufferSize;
        m_nextAssetReady = false;
        return asset;
    }

private:
    dispatch_semaphore_t m_semWriteAsset;
    dispatch_semaphore_t m_semReadAsset;
    std::atomic_bool m_stop;

    ALAssetsLibrary *m_assetsLibrary;
    ALAssetsGroupType m_type;
    QVector<ALAsset *> m_buffer;
    int m_readIndex;
    int m_writeIndex;
    bool m_nextAssetReady;

    void writeAsset(ALAsset *asset)
    {
        dispatch_semaphore_wait(m_semWriteAsset, DISPATCH_TIME_FOREVER);
        m_buffer[m_writeIndex] = [asset retain];
        dispatch_semaphore_signal(m_semReadAsset);
        m_writeIndex = (m_writeIndex + 1) % kBufferSize;
    }

    void startEnumerate()
    {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [m_assetsLibrary enumerateGroupsWithTypes:m_type usingBlock:^(ALAssetsGroup *group, BOOL *stopEnumerate) {

                if (!group) {
                    writeAsset(kNoAsset);
                    return;
                }

                if (m_stop) {
                    *stopEnumerate = true;
                    return;
                }

                [group enumerateAssetsUsingBlock:^(ALAsset *asset, NSUInteger index, BOOL *stopEnumerate) {
                    Q_UNUSED(index);
                    if (!asset || ![[asset valueForProperty:ALAssetPropertyType] isEqual:ALAssetTypePhoto])
                       return;

                    writeAsset(asset);
                    *stopEnumerate = m_stop;
                }];
            } failureBlock:^(NSError *error) {
                NSLog(@"QIOSFileEngine: %@", error);
                writeAsset(kNoAsset);
            }];
        });
    }

};

// -------------------------------------------------------------------------

class QIOSAssetData : public QObject
{
public:
    QIOSAssetData(const QString &assetUrl, QIOSFileEngineAssetsLibrary *engine)
        : m_asset(0)
        , m_assetUrl(assetUrl)
        , m_assetLibrary(0)
    {
        if (!ensureAuthorizationDialogNotBlocked())
            return;

        if (QIOSAssetData *assetData = g_assetDataCache.localData()) {
            // It's a common pattern that QFiles pointing to the same path are created and destroyed
            // several times during a single event loop cycle. To avoid loading the same asset
            // over and over, we check if the last loaded asset has not been destroyed yet, and try to
            // reuse its data.
            if (assetData->m_assetUrl == assetUrl) {
                m_assetLibrary = [assetData->m_assetLibrary retain];
                m_asset = [assetData->m_asset retain];
                return;
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

                if (!asset) {
                    // When an asset couldn't be loaded, chances are that it belongs to ALAssetsGroupPhotoStream.
                    // Such assets can be stored in the cloud and might need to be downloaded first. Unfortunately,
                    // forcing that to happen is hidden behind private APIs ([ALAsset requestDefaultRepresentation]).
                    // As a work-around, we search for it instead, since that will give us a pointer to the asset.
                    QIOSAssetEnumerator e(m_assetLibrary, ALAssetsGroupPhotoStream);
                    while (e.hasNext()) {
                        ALAsset *a = e.next();
                        QString url = QUrl::fromNSURL([a valueForProperty:ALAssetPropertyAssetURL]).toString();
                        if (url == assetUrl) {
                            asset = a;
                            break;
                        }
                    }
                }

                if (!asset)
                    engine->setError(QFile::OpenError, QLatin1String("could not open image"));

                m_asset = [asset retain];
                dispatch_semaphore_signal(semaphore);
            } failureBlock:^(NSError *error) {
                engine->setError(QFile::OpenError, QString::fromNSString(error.localizedDescription));
                dispatch_semaphore_signal(semaphore);
            }];
        });

        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        dispatch_release(semaphore);

        g_assetDataCache.setLocalData(this);
    }

    ~QIOSAssetData()
    {
        [m_assetLibrary release];
        [m_asset release];
        if (g_assetDataCache.localData() == this)
            g_assetDataCache.setLocalData(0);
    }

    ALAsset *m_asset;

private:
    QString m_assetUrl;
    ALAssetsLibrary *m_assetLibrary;
};

// -------------------------------------------------------------------------

#ifndef QT_NO_FILESYSTEMITERATOR

class QIOSFileEngineIteratorAssetsLibrary : public QAbstractFileEngineIterator
{
public:
    QIOSAssetEnumerator *m_enumerator;

    QIOSFileEngineIteratorAssetsLibrary(
            QDir::Filters filters, const QStringList &nameFilters)
        : QAbstractFileEngineIterator(filters, nameFilters)
        , m_enumerator(new QIOSAssetEnumerator([[[ALAssetsLibrary alloc] init] autorelease], ALAssetsGroupAll))
    {
    }

    ~QIOSFileEngineIteratorAssetsLibrary()
    {
        delete m_enumerator;
        g_iteratorCurrentUrl.setLocalData(QString());
    }

    QString next() Q_DECL_OVERRIDE
    {
        // Cache the URL that we are about to return, since QDir will immediately create a
        // new file engine on the file and ask if it exists. Unless we do this, we end up
        // creating a new ALAsset just to verify its existence, which will be especially
        // costly for assets belonging to ALAssetsGroupPhotoStream.
        ALAsset *asset = m_enumerator->next();
        QString url = QUrl::fromNSURL([asset valueForProperty:ALAssetPropertyAssetURL]).toString();
        g_iteratorCurrentUrl.setLocalData(url);
        return url;
    }

    bool hasNext() const Q_DECL_OVERRIDE
    {
        return m_enumerator->hasNext();
    }

    QString currentFileName() const Q_DECL_OVERRIDE
    {
        return g_iteratorCurrentUrl.localData();
    }

    QFileInfo currentFileInfo() const Q_DECL_OVERRIDE
    {
        return QFileInfo(currentFileName());
    }
};

#endif

// -------------------------------------------------------------------------

QIOSFileEngineAssetsLibrary::QIOSFileEngineAssetsLibrary(const QString &fileName)
    : m_offset(0)
    , m_data(0)
{
    setFileName(fileName);
}

QIOSFileEngineAssetsLibrary::~QIOSFileEngineAssetsLibrary()
{
    close();
}

ALAsset *QIOSFileEngineAssetsLibrary::loadAsset() const
{
    if (!m_data)
        m_data = new QIOSAssetData(m_assetUrl, const_cast<QIOSFileEngineAssetsLibrary *>(this));
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
    const bool isDir = (m_assetUrl == QLatin1String("assets-library://"));
    const bool exists = isDir || m_assetUrl == g_iteratorCurrentUrl.localData() || loadAsset();

    if (!exists)
        return flags;

    if (type & FlagsMask)
        flags |= ExistsFlag;
    if (type & PermsMask) {
        ALAuthorizationStatus status = [ALAssetsLibrary authorizationStatus];
        if (status != ALAuthorizationStatusRestricted && status != ALAuthorizationStatusDenied)
            flags |= ReadOwnerPerm | ReadUserPerm | ReadGroupPerm | ReadOtherPerm;
    }
    if (type & TypesMask)
        flags |= isDir ? DirectoryType : FileType;

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
    // QUrl::fromLocalFile() will remove double slashes. Since the asset url is
    // passed around as a file name in the app (and converted to/from a file url, e.g
    // in QFileDialog), we need to ensure that m_assetUrl ends up being valid.
    int index = file.indexOf(QLatin1String("/asset"));
    if (index == -1)
        m_assetUrl = QLatin1String("assets-library://");
    else
        m_assetUrl = QLatin1String("assets-library:/") + file.mid(index);
}

QStringList QIOSFileEngineAssetsLibrary::entryList(QDir::Filters filters, const QStringList &filterNames) const
{
    return QAbstractFileEngine::entryList(filters, filterNames);
}

#ifndef QT_NO_FILESYSTEMITERATOR

QAbstractFileEngine::Iterator *QIOSFileEngineAssetsLibrary::beginEntryList(
        QDir::Filters filters, const QStringList &filterNames)
{
    return new QIOSFileEngineIteratorAssetsLibrary(filters, filterNames);
}

QAbstractFileEngine::Iterator *QIOSFileEngineAssetsLibrary::endEntryList()
{
    return 0;
}

#endif
