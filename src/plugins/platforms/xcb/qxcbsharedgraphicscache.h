/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QXCBSHAREDGRAPHICSCACHE
#define QXCBSHAREDGRAPHICSCACHE

#if defined(QT_USE_XCB_SHARED_GRAPHICS_CACHE)

#include <QtGui/qplatformsharedgraphicscache_qpa.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QXcbSharedBufferManager;
class QXcbSharedGraphicsCache : public QPlatformSharedGraphicsCache
{
    Q_OBJECT
public:
    explicit QXcbSharedGraphicsCache(QObject *parent = 0);

    virtual void ensureCacheInitialized(const QByteArray &cacheId, BufferType bufferType,
                                        PixelFormat pixelFormat);

    virtual void requestItems(const QByteArray &cacheId, const QVector<quint32> &itemIds);
    virtual void insertItems(const QByteArray &cacheId,
                             const QVector<quint32> &itemIds,
                             const QVector<QImage> &items);
    virtual void releaseItems(const QByteArray &cacheId, const QVector<quint32> &itemIds);

    virtual void serializeBuffer(void *bufferId, QByteArray *serializedData, int *fileDescriptor) const;
    virtual uint textureIdForBuffer(void *bufferId);
    virtual void referenceBuffer(void *bufferId);
    virtual bool dereferenceBuffer(void *bufferId);

private:
    struct ReadyItem {
        QVector<quint32> itemIds;
        QVector<QPoint> positions;
    };

    void processPendingItems();

    QXcbSharedBufferManager *m_bufferManager;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_USE_XCB_SHARED_GRAPHICS_CACHE

#endif // QXCBSHAREDGRAPHICSCACHE
