/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#if defined(QT_USE_XCB_SHARED_GRAPHICS_CACHE)

#include "qxcbsharedgraphicscache.h"
#include "qxcbsharedbuffermanager.h"

#include <QtCore/qsharedmemory.h>

#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>

#define SHAREDGRAPHICSCACHE_DEBUG 1

QT_BEGIN_NAMESPACE

QXcbSharedGraphicsCache::QXcbSharedGraphicsCache(QObject *parent)
    : QPlatformSharedGraphicsCache(parent)
    , m_bufferManager(new QXcbSharedBufferManager)
{
}

void QXcbSharedGraphicsCache::requestItems(const QByteArray &cacheId,
                                                  const QVector<quint32> &itemIds)
{
    m_bufferManager->beginSharedBufferAction(cacheId);

    QSet<quint32> itemsForRequest;
    for (int i=0; i<itemIds.size(); ++i)
        itemsForRequest.insert(itemIds.at(i));

    m_bufferManager->requestItems(itemsForRequest);
    m_bufferManager->endSharedBufferAction();

    processPendingItems();
}

void QXcbSharedGraphicsCache::insertItems(const QByteArray &cacheId,
                                                 const QVector<quint32> &itemIds,
                                                 const QVector<QImage> &items)
{
    m_bufferManager->beginSharedBufferAction(cacheId);

    QSet<quint32> itemsForRequest;
    for (int i=0; i<itemIds.size(); ++i) {
        QImage image = items.at(i);
        m_bufferManager->insertItem(itemIds.at(i), image.bits(), image.width(), image.height());
        itemsForRequest.insert(itemIds.at(i));
    }

    // ### To avoid loops, we could check missing items here and notify the client
    m_bufferManager->requestItems(itemsForRequest);

    m_bufferManager->endSharedBufferAction();

    processPendingItems();
}

void QXcbSharedGraphicsCache::ensureCacheInitialized(const QByteArray &cacheId,
                                                            BufferType bufferType,
                                                            PixelFormat pixelFormat)
{
    Q_UNUSED(cacheId);
    Q_UNUSED(bufferType);
    Q_UNUSED(pixelFormat);
}


void QXcbSharedGraphicsCache::releaseItems(const QByteArray &cacheId,
                                                  const QVector<quint32> &itemIds)
{
    m_bufferManager->beginSharedBufferAction(cacheId);

    QSet<quint32> itemsToRelease;
    for (int i=0; i<itemIds.size(); ++i)
        itemsToRelease.insert(itemIds.at(i));

    m_bufferManager->releaseItems(itemsToRelease);

    m_bufferManager->endSharedBufferAction();

    processPendingItems();
}

void QXcbSharedGraphicsCache::serializeBuffer(void *bufferId,
                                                     QByteArray *serializedData,
                                                     int *fileDescriptor) const
{
    QXcbSharedBufferManager::Buffer *buffer =
            reinterpret_cast<QXcbSharedBufferManager::Buffer *>(bufferId);

    QPair<QByteArray, int> bufferName = m_bufferManager->serializeBuffer(buffer->buffer);

    *serializedData = bufferName.first;
    *fileDescriptor = bufferName.second;
}

GLuint QXcbSharedGraphicsCache::textureIdForBuffer(void *bufferId)
{
#  if defined(SHAREDGRAPHICSCACHE_DEBUG)
    qDebug("QXcbSharedGraphicsCache::textureIdForBuffer");
#  endif

    QXcbSharedBufferManager::Buffer *buffer =
            reinterpret_cast<QXcbSharedBufferManager::Buffer *>(bufferId);

    if (buffer->textureId == 0) {
        glGenTextures(1, &buffer->textureId);
        if (buffer->textureId == 0) {
            qWarning("QXcbSharedGraphicsCache::textureIdForBuffer: Failed to generate texture (gl error: 0x%x)",
                     glGetError());
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, buffer->textureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindTexture(GL_TEXTURE_2D, buffer->textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, buffer->width, buffer->height, 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, buffer->buffer->data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return buffer->textureId;
}

void QXcbSharedGraphicsCache::referenceBuffer(void *bufferId)
{
    QXcbSharedBufferManager::Buffer *buffer =
        reinterpret_cast<QXcbSharedBufferManager::Buffer *>(bufferId);

    buffer->ref.ref();
}

bool QXcbSharedGraphicsCache::dereferenceBuffer(void *bufferId)
{
    QXcbSharedBufferManager::Buffer *buffer =
            reinterpret_cast<QXcbSharedBufferManager::Buffer *>(bufferId);

    if (buffer->ref.deref())
        return true;

    if (buffer->textureId != 0) {
        glDeleteTextures(1, &buffer->textureId);
        buffer->textureId = 0;
    }

    return false;
}

void QXcbSharedGraphicsCache::processPendingItems()
{
#  if defined(SHAREDGRAPHICSCACHE_DEBUG)
        qDebug("QXcbSharedGraphicsCache::processPendingItems");
#  endif

    {
        QXcbSharedBufferManager::PendingItemIds pendingMissingItems = m_bufferManager->pendingItemsMissing();
        QXcbSharedBufferManager::PendingItemIds::const_iterator it;


        for (it = pendingMissingItems.constBegin(); it != pendingMissingItems.constEnd(); ++it) {
            QVector<quint32> missingItems;

            const QSet<quint32> &items = it.value();
            QSet<quint32>::const_iterator itemIt;
            for (itemIt = items.constBegin(); itemIt != items.constEnd(); ++itemIt)
                missingItems.append(*itemIt);

#  if defined(SHAREDGRAPHICSCACHE_DEBUG)
        qDebug("QXcbSharedGraphicsCache::processPendingItems: %d missing items",
               missingItems.size());
#  endif

            if (!missingItems.isEmpty())
                emit itemsMissing(it.key(), missingItems);
        }
    }

    {
        QXcbSharedBufferManager::PendingItemIds pendingInvalidatedItems = m_bufferManager->pendingItemsInvalidated();
        QXcbSharedBufferManager::PendingItemIds::const_iterator it;

        for (it = pendingInvalidatedItems.constBegin(); it != pendingInvalidatedItems.constEnd(); ++it) {
            QVector<quint32> invalidatedItems;

            const QSet<quint32> &items = it.value();
            QSet<quint32>::const_iterator itemIt;
            for (itemIt = items.constBegin(); itemIt != items.constEnd(); ++itemIt)
                invalidatedItems.append(*itemIt);

#  if defined(SHAREDGRAPHICSCACHE_DEBUG)
            qDebug("QXcbSharedGraphicsCache::processPendingItems: %d invalidated items",
               invalidatedItems.size());
#  endif

            if (!invalidatedItems.isEmpty())
                emit itemsInvalidated(it.key(), invalidatedItems);
        }
    }

    {
        QXcbSharedBufferManager::PendingItemIds pendingReadyItems = m_bufferManager->pendingItemsReady();
        QXcbSharedBufferManager::PendingItemIds::const_iterator it;

        for (it = pendingReadyItems.constBegin(); it != pendingReadyItems.constEnd(); ++it) {
            QHash<QXcbSharedBufferManager::Buffer *, ReadyItem> readyItemsForBuffer;
            const QSet<quint32> &items = it.value();

            QByteArray cacheId = it.key();

            QSet<quint32>::const_iterator itemIt;
            for (itemIt = items.constBegin(); itemIt != items.constEnd(); ++itemIt) {
                QXcbSharedBufferManager::Buffer *buffer;
                int x = -1;
                int y = -1;

                m_bufferManager->getBufferForItem(cacheId, *itemIt, &buffer, &x, &y);

                readyItemsForBuffer[buffer].itemIds.append(*itemIt);
                readyItemsForBuffer[buffer].positions.append(QPoint(x, y));
            }

            QHash<QXcbSharedBufferManager::Buffer*, ReadyItem>::iterator readyItemIt
                    = readyItemsForBuffer.begin();
            while (readyItemIt != readyItemsForBuffer.end()) {
                QXcbSharedBufferManager::Buffer *buffer = readyItemIt.key();
                if (!readyItemIt.value().itemIds.isEmpty()) {
#  if defined(SHAREDGRAPHICSCACHE_DEBUG)
                    qDebug("QXcbSharedGraphicsCache::processPendingItems: %d ready items",
                            readyItemIt.value().itemIds.size());
#  endif

                    emit itemsAvailable(cacheId, buffer, QSize(buffer->width, buffer->height),
                                        readyItemIt.value().itemIds, readyItemIt.value().positions);
                }
                ++readyItemIt;
            }
        }
    }
}

QT_END_NAMESPACE

#endif // QT_USE_XCB_SHARED_GRAPHICS_CACHE
