/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#if defined(QT_USE_XCB_SHARED_GRAPHICS_CACHE)

#include "qxcbsharedbuffermanager.h"

#include <QtCore/quuid.h>
#include <QtGui/qimage.h>

#include <stdio.h>

#if !defined(SHAREDGRAPHICSCACHE_MAX_MEMORY_USED)
#  define SHAREDGRAPHICSCACHE_MAX_MEMORY_USED 16 * 1024 * 1024 // 16 MB limit
#endif

#if !defined(SHAREDGRAPHICSCACHE_MAX_TEXTURES_PER_CACHE)
#  define SHAREDGRAPHICSCACHE_MAX_TEXTURES_PER_CACHE 1
#endif

#if !defined(SHAREDGRAPHICSCACHE_TEXTURE_SIZE)
#  define SHAREDGRAPHICSCACHE_TEXTURE_SIZE 2048
#endif

#define SHAREDBUFFERMANAGER_DEBUG 1

QT_BEGIN_NAMESPACE

QXcbSharedBufferManager::QXcbSharedBufferManager()
    : m_memoryUsed(0)
    , m_mostRecentlyUsed(0)
    , m_leastRecentlyUsed(0)
{
}

QXcbSharedBufferManager::~QXcbSharedBufferManager()
{
    {
        QHash<QByteArray, Buffer *>::const_iterator it = m_buffers.constBegin();
        while (it != m_buffers.constEnd()) {
            Buffer *buffer = it.value();
            delete buffer;
            ++it;
        }
    }

    {
        QHash<QByteArray, Items *>::const_iterator it = m_items.constBegin();
        while (it != m_items.constEnd()) {
            Items *items = it.value();
            QHash<quint32, Item *>::const_iterator itemIt = items->items.constBegin();
            while (itemIt != items->items.constEnd()) {
                delete itemIt.value();
                ++itemIt;
            }
            delete it.value();
            ++it;
        }
    }
}

void QXcbSharedBufferManager::getBufferForItem(const QByteArray &cacheId, quint32 itemId,
                                                      Buffer **buffer, int *x, int *y) const
{
    Q_ASSERT_X(m_currentCacheId.isEmpty(), Q_FUNC_INFO,
               "Call endSharedBufferAction before accessing data");

    Q_ASSERT(buffer != 0);
    Q_ASSERT(x != 0);
    Q_ASSERT(y != 0);

    Items *items = itemsForCache(cacheId);
    Item *item = items->items.value(itemId);
    if (item != 0) {
        *buffer = item->buffer;
        *x = item->x;
        *y = item->y;
    } else {
        *buffer = 0;
        *x = -1;
        *y = -1;
    }
}

QPair<QByteArray, int> QXcbSharedBufferManager::serializeBuffer(QSharedMemory *buffer) const
{
    Q_ASSERT_X(m_currentCacheId.isEmpty(), Q_FUNC_INFO,
               "Call endSharedBufferAction before accessing data");

    return qMakePair(buffer->key().toLatin1(), 0);
}

void QXcbSharedBufferManager::beginSharedBufferAction(const QByteArray &cacheId)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::beginSharedBufferAction() called for %s", cacheId.constData());
#endif

    Q_ASSERT(m_currentCacheId.isEmpty());
    Q_ASSERT(!cacheId.isEmpty());

    m_pendingInvalidatedItems.clear();
    m_pendingReadyItems.clear();
    m_pendingMissingItems.clear();

    m_currentCacheId = cacheId;
}

void QXcbSharedBufferManager::requestItems(const QSet<quint32> &itemIds)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::requestItems for %d items", itemIds.size());
#endif

    Q_ASSERT_X(!m_currentCacheId.isEmpty(), Q_FUNC_INFO,
               "Call beginSharedBufferAction before requesting items");
    Items *items = itemsForCache(m_currentCacheId);

    QSet<quint32>::const_iterator it = itemIds.constBegin();
    while (it != itemIds.constEnd()) {
        if (items->items.contains(*it))
            m_pendingReadyItems[m_currentCacheId].insert(*it);
        else
            m_pendingMissingItems[m_currentCacheId].insert(*it);
        ++it;
    }
}

void QXcbSharedBufferManager::releaseItems(const QSet<quint32> &itemIds)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::releaseItems for %d items", itemIds.size());
#endif

    Items *items = itemsForCache(m_currentCacheId);

    QSet<quint32>::const_iterator it;
    for (it = itemIds.constBegin(); it != itemIds.constEnd(); ++it) {
        Item *item = items->items.value(*it);
        if (item != 0)
            pushItemToBack(items, item);

        m_pendingReadyItems[m_currentCacheId].remove(*it);
        m_pendingMissingItems[m_currentCacheId].remove(*it);
    }
}

void QXcbSharedBufferManager::insertItem(quint32 itemId, uchar *data,
                                                int itemWidth, int itemHeight)
{
    Q_ASSERT_X(!m_currentCacheId.isEmpty(), Q_FUNC_INFO,
               "Call beginSharedBufferAction before inserting items");
    Items *items = itemsForCache(m_currentCacheId);

    if (!items->items.contains(itemId)) {
        Buffer *sharedBuffer = 0;
        int x = 0;
        int y = 0;

        findAvailableBuffer(itemWidth, itemHeight, &sharedBuffer, &x, &y);
        copyIntoBuffer(sharedBuffer, x, y, itemWidth, itemHeight, data);

//        static int counter=0;
//        QString fileName = QString::fromLatin1("buffer%1.png").arg(counter++);
//        saveBuffer(sharedBuffer, fileName);

        Item *item = new Item;
        item->itemId = itemId;
        item->buffer = sharedBuffer;
        item->x = x;
        item->y = y;

        items->items[itemId] = item;

        touchItem(items, item);
    }
}

void QXcbSharedBufferManager::endSharedBufferAction()
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::endSharedBufferAction() called for %s",
           m_currentCacheId.constData());
#endif

    Q_ASSERT(!m_currentCacheId.isEmpty());

    // Do an extra validation pass on the invalidated items since they may have been re-inserted
    // after they were invalidated
    if (m_pendingInvalidatedItems.contains(m_currentCacheId)) {
        QSet<quint32> &invalidatedItems = m_pendingInvalidatedItems[m_currentCacheId];
        QSet<quint32>::iterator it = invalidatedItems.begin();
        while (it != invalidatedItems.end()) {
            Items *items = m_items.value(m_currentCacheId);

            if (items->items.contains(*it)) {
                m_pendingReadyItems[m_currentCacheId].insert(*it);
                it = invalidatedItems.erase(it);
            } else {
                ++it;
            }
        }
    }

    m_currentCacheId.clear();
}

void QXcbSharedBufferManager::pushItemToBack(Items *items, Item *item)
{
    if (items->leastRecentlyUsed == item)
        return;

    if (item->next != 0)
        item->next->prev = item->prev;
    if (item->prev != 0)
        item->prev->next = item->next;

    if (items->mostRecentlyUsed == item)
        items->mostRecentlyUsed = item->prev;

    if (items->leastRecentlyUsed != 0)
        items->leastRecentlyUsed->prev = item;

    item->prev = 0;
    item->next = items->leastRecentlyUsed;
    items->leastRecentlyUsed = item;
    if (items->mostRecentlyUsed == 0)
        items->mostRecentlyUsed = item;
}

void QXcbSharedBufferManager::touchItem(Items *items, Item *item)
{
    if (items->mostRecentlyUsed == item)
        return;

    if (item->next != 0)
        item->next->prev = item->prev;
    if (item->prev != 0)
        item->prev->next = item->next;

    if (items->leastRecentlyUsed == item)
        items->leastRecentlyUsed = item->next;

    if (items->mostRecentlyUsed != 0)
        items->mostRecentlyUsed->next = item;

    item->next = 0;
    item->prev = items->mostRecentlyUsed;
    items->mostRecentlyUsed = item;
    if (items->leastRecentlyUsed == 0)
        items->leastRecentlyUsed = item;
}

void QXcbSharedBufferManager::deleteItem(Items *items, Item *item)
{
    Q_ASSERT(items != 0);
    Q_ASSERT(item != 0);

    if (items->mostRecentlyUsed == item)
        items->mostRecentlyUsed = item->prev;
    if (items->leastRecentlyUsed == item)
        items->leastRecentlyUsed = item->next;

    if (item->next != 0)
        item->next->prev = item->prev;
    if (item->prev != 0)
        item->prev->next = item->next;

    m_pendingInvalidatedItems[items->cacheId].insert(item->itemId);

    {
        QHash<quint32, Item *>::iterator it = items->items.find(item->itemId);
        while (it != items->items.end() && it.value()->itemId == item->itemId)
            it = items->items.erase(it);
    }

    delete item;
}

void QXcbSharedBufferManager::recycleItem(Buffer **sharedBuffer, int *glyphX, int *glyphY)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::recycleItem() called for %s", m_currentCacheId.constData());
#endif

    Items *items = itemsForCache(m_currentCacheId);

    Item *recycledItem = items->leastRecentlyUsed;
    Q_ASSERT(recycledItem != 0);

    *sharedBuffer = recycledItem->buffer;
    *glyphX = recycledItem->x;
    *glyphY = recycledItem->y;

    deleteItem(items, recycledItem);
}

void QXcbSharedBufferManager::touchBuffer(Buffer *buffer)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::touchBuffer() called for %s", buffer->cacheId.constData());
#endif

    if (buffer == m_mostRecentlyUsed)
        return;

    if (buffer->next != 0)
        buffer->next->prev = buffer->prev;
    if (buffer->prev != 0)
        buffer->prev->next = buffer->next;

    if (m_leastRecentlyUsed == buffer)
        m_leastRecentlyUsed = buffer->next;

    buffer->next = 0;
    buffer->prev = m_mostRecentlyUsed;
    if (m_mostRecentlyUsed != 0)
        m_mostRecentlyUsed->next = buffer;
    if (m_leastRecentlyUsed == 0)
        m_leastRecentlyUsed = buffer;
    m_mostRecentlyUsed = buffer;
}

void QXcbSharedBufferManager::deleteLeastRecentlyUsed()
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::deleteLeastRecentlyUsed() called");
#endif

    if (m_leastRecentlyUsed == 0)
        return;

    Buffer *old = m_leastRecentlyUsed;
    m_leastRecentlyUsed = old->next;
    m_leastRecentlyUsed->prev = 0;

    QByteArray cacheId = old->cacheId;
    Items *items = itemsForCache(cacheId);

    QHash<quint32, Item *>::iterator it = items->items.begin();
    while (it != items->items.end()) {
        Item *item = it.value();
        if (item->buffer == old) {
            deleteItem(items, item);
            it = items->items.erase(it);
        } else {
            ++it;
        }
    }

    m_buffers.remove(cacheId, old);
    m_memoryUsed -= old->width * old->height * old->bytesPerPixel;

#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::deleteLeastRecentlyUsed: Memory used: %d / %d (%6.2f %%)",
           m_memoryUsed, SHAREDGRAPHICSCACHE_MAX_MEMORY_USED,
           100.0f * float(m_memoryUsed) / float(SHAREDGRAPHICSCACHE_MAX_MEMORY_USED));
#endif

    delete old;
}

QXcbSharedBufferManager::Buffer *QXcbSharedBufferManager::createNewBuffer(const QByteArray &cacheId,
                                                                  int heightRequired)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::createNewBuffer() called for %s", cacheId.constData());
#endif

    // ###
    // if (bufferCount of cacheId == SHAREDGRAPHICACHE_MAX_TEXTURES_PER_CACHE)
    //    deleteLeastRecentlyUsedBufferForCache(cacheId);

    // ### Take pixel format into account
    while (m_memoryUsed + SHAREDGRAPHICSCACHE_TEXTURE_SIZE * heightRequired >= SHAREDGRAPHICSCACHE_MAX_MEMORY_USED)
        deleteLeastRecentlyUsed();

    Buffer *buffer = allocateBuffer(SHAREDGRAPHICSCACHE_TEXTURE_SIZE, heightRequired);
    buffer->cacheId = cacheId;

    buffer->currentLineMaxHeight = 0;
    m_buffers.insert(cacheId, buffer);

    return buffer;
}

static inline int qt_next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

QXcbSharedBufferManager::Buffer *QXcbSharedBufferManager::resizeBuffer(Buffer *oldBuffer, const QSize &newSize)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::resizeBuffer() called for %s (current size: %dx%d, new size: %dx%d)",
           oldBuffer->cacheId.constData(), oldBuffer->width, oldBuffer->height,
           newSize.width(), newSize.height());
#endif

    // Remove old buffer from lists to avoid deleting it under our feet
    if (m_leastRecentlyUsed == oldBuffer)
        m_leastRecentlyUsed = oldBuffer->next;
    if (m_mostRecentlyUsed == oldBuffer)
        m_mostRecentlyUsed = oldBuffer->prev;

    if (oldBuffer->prev != 0)
        oldBuffer->prev->next = oldBuffer->next;
    if (oldBuffer->next != 0)
        oldBuffer->next->prev = oldBuffer->prev;

    m_memoryUsed -= oldBuffer->width * oldBuffer->height * oldBuffer->bytesPerPixel;
    m_buffers.remove(oldBuffer->cacheId, oldBuffer);

#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::resizeBuffer:            Memory used: %d / %d (%6.2f %%)",
           m_memoryUsed, SHAREDGRAPHICSCACHE_MAX_MEMORY_USED,
           100.0f * float(m_memoryUsed) / float(SHAREDGRAPHICSCACHE_MAX_MEMORY_USED));
#endif

    Buffer *resizedBuffer = createNewBuffer(oldBuffer->cacheId, newSize.height());
    copyIntoBuffer(resizedBuffer, 0, 0, oldBuffer->width, oldBuffer->height,
                   reinterpret_cast<uchar *>(oldBuffer->buffer->data()));

    resizedBuffer->currentLineMaxHeight = oldBuffer->currentLineMaxHeight;

    Items *items = itemsForCache(oldBuffer->cacheId);
    QHash<quint32, Item *>::const_iterator it = items->items.constBegin();
    while (it != items->items.constEnd()) {
        Item *item = it.value();
        if (item->buffer == oldBuffer) {
            m_pendingReadyItems[oldBuffer->cacheId].insert(item->itemId);
            item->buffer = resizedBuffer;
        }
        ++it;
    }

    resizedBuffer->nextX = oldBuffer->nextX;
    resizedBuffer->nextY = oldBuffer->nextY;
    resizedBuffer->currentLineMaxHeight = oldBuffer->currentLineMaxHeight;

    delete oldBuffer;
    return resizedBuffer;
}

void QXcbSharedBufferManager::findAvailableBuffer(int itemWidth, int itemHeight,
                                              Buffer **sharedBuffer, int *glyphX, int *glyphY)
{
    Q_ASSERT(sharedBuffer != 0);
    Q_ASSERT(glyphX != 0);
    Q_ASSERT(glyphY != 0);

    QMultiHash<QByteArray, Buffer *>::iterator it = m_buffers.find(m_currentCacheId);

    int bufferCount = 0;
    while (it != m_buffers.end() && it.key() == m_currentCacheId) {
        Buffer *buffer = it.value();

        int x = buffer->nextX;
        int y = buffer->nextY;
        int width = buffer->width;
        int height = buffer->height;

        if (x + itemWidth <= width && y + itemHeight <= height) {
            // There is space on the current line, put the item there
            buffer->currentLineMaxHeight = qMax(buffer->currentLineMaxHeight, itemHeight);
            *sharedBuffer = buffer;
            *glyphX = x;
            *glyphY = y;

            buffer->nextX += itemWidth;

            return;
        } else if (itemWidth <= width && y + buffer->currentLineMaxHeight + itemHeight <= height) {
            // There is space for a new line, put the item on the new line
            buffer->nextX = 0;
            buffer->nextY += buffer->currentLineMaxHeight;
            buffer->currentLineMaxHeight = 0;

            *sharedBuffer = buffer;
            *glyphX = buffer->nextX;
            *glyphY = buffer->nextY;

            buffer->nextX += itemWidth;

            return;
        } else if (y + buffer->currentLineMaxHeight + itemHeight <= SHAREDGRAPHICSCACHE_TEXTURE_SIZE) {
            // There is space if we resize the buffer, so we do that
            int newHeight = qt_next_power_of_two(y + buffer->currentLineMaxHeight + itemHeight);
            buffer = resizeBuffer(buffer, QSize(width, newHeight));

            buffer->nextX = 0;
            buffer->nextY += buffer->currentLineMaxHeight;
            buffer->currentLineMaxHeight = 0;

            *sharedBuffer = buffer;
            *glyphX = buffer->nextX;
            *glyphY = buffer->nextY;

            buffer->nextX += itemWidth;
            return;
        }

        bufferCount++;
        ++it;
    }

    if (bufferCount == SHAREDGRAPHICSCACHE_MAX_TEXTURES_PER_CACHE) {
        // There is no space in any buffer, and there is no space for a new buffer
        // recycle an old item
        recycleItem(sharedBuffer, glyphX, glyphY);
    } else {
        // Create a new buffer for the item
        *sharedBuffer = createNewBuffer(m_currentCacheId, qt_next_power_of_two(itemHeight));
        if (*sharedBuffer == 0) {
            Q_ASSERT(false);
            return;
        }

        *glyphX = (*sharedBuffer)->nextX;
        *glyphY = (*sharedBuffer)->nextY;

        (*sharedBuffer)->nextX += itemWidth;
    }
}

QXcbSharedBufferManager::Buffer *QXcbSharedBufferManager::allocateBuffer(int width, int height)
{
    Buffer *buffer = new Buffer;
    buffer->nextX = 0;
    buffer->nextY = 0;
    buffer->width = width;
    buffer->height = height;
    buffer->bytesPerPixel = 1; // ### Use pixel format here

    buffer->buffer = new QSharedMemory(QUuid::createUuid().toString());
    bool ok = buffer->buffer->create(buffer->width * buffer->height * buffer->bytesPerPixel,
                                     QSharedMemory::ReadWrite);
    if (!ok) {
        qWarning("QXcbSharedBufferManager::findAvailableBuffer: Can't create new buffer (%s)",
                 qPrintable(buffer->buffer->errorString()));
        delete buffer;
        return 0;
    }
    qMemSet(buffer->buffer->data(), 0, buffer->buffer->size());

    m_memoryUsed += buffer->width * buffer->height * buffer->bytesPerPixel;

#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::allocateBuffer:          Memory used: %d / %d (%6.2f %%)",
           int(m_memoryUsed), int(SHAREDGRAPHICSCACHE_MAX_MEMORY_USED),
           100.0f * float(m_memoryUsed) / float(SHAREDGRAPHICSCACHE_MAX_MEMORY_USED));
#endif

    return buffer;
}

void QXcbSharedBufferManager::copyIntoBuffer(Buffer *buffer,
                                                    int bufferX, int bufferY, int width, int height,
                                                    uchar *data)
{
#if defined(SHAREDBUFFERMANAGER_DEBUG)
    qDebug("QXcbSharedBufferManager::copyIntoBuffer() called for %s (coords: %d, %d)",
           buffer->cacheId.constData(), bufferX, bufferY);
#endif

    Q_ASSERT(bufferX >= 0);
    Q_ASSERT(bufferX + width <= buffer->width);
    Q_ASSERT(bufferY >= 0);
    Q_ASSERT(bufferY + height <= buffer->height);

    uchar *dest = reinterpret_cast<uchar *>(buffer->buffer->data());
    dest += bufferX + bufferY * buffer->width;
    for (int y=0; y<height; ++y) {
        qMemCopy(dest, data, width);

        data += width;
        dest += buffer->width;
    }
}

QXcbSharedBufferManager::Items *QXcbSharedBufferManager::itemsForCache(const QByteArray &cacheId) const
{
    Items *items = m_items.value(cacheId);
    if (items == 0) {
        items = new Items;
        items->cacheId = cacheId;
        m_items[cacheId] = items;
    }

    return items;
}

QT_END_NAMESPACE

#endif // QT_USE_XCB_SHARED_GRAPHICS_CACHE
