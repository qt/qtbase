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

#ifndef XCBSHAREDBUFFERMANAGER_H
#define XCBSHAREDBUFFERMANAGER_H

#if defined(QT_USE_XCB_SHARED_GRAPHICS_CACHE)

#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtCore/qsharedmemory.h>

#include <GLES2/gl2.h>

#include <EGL/egl.h>

#include <EGL/eglext.h>

class wl_resource;

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QXcbSharedBufferManager
{
public:
    struct Buffer {
        Buffer()
            : width(-1)
            , height(-1)
            , bytesPerPixel(1)
            , nextX(-1)
            , nextY(-1)
            , currentLineMaxHeight(0)
            , next(0)
            , prev(0)
            , buffer(0)
            , textureId(0)
        {
        }

        ~Buffer()
        {
            delete buffer;

            if (textureId != 0)
                glDeleteTextures(1, &textureId);
        }

        QByteArray cacheId;
        int width;
        int height;
        int bytesPerPixel;
        int nextX;
        int nextY;
        int currentLineMaxHeight;

        Buffer *next;
        Buffer *prev;

        QSharedMemory *buffer;

        GLuint textureId;

        QAtomicInt ref;
    };

    typedef QHash<QByteArray, QSet<quint32> > PendingItemIds;

    QXcbSharedBufferManager();
    ~QXcbSharedBufferManager();

    void beginSharedBufferAction(const QByteArray &cacheId);
    void insertItem(quint32 itemId, uchar *data, int itemWidth, int itemHeight);
    void requestItems(const QSet<quint32> &itemIds);
    void releaseItems(const QSet<quint32> &itemIds);
    void endSharedBufferAction();

    void getBufferForItem(const QByteArray &cacheId, quint32 itemId, Buffer **buffer,
                          int *x, int *y) const;
    QPair<QByteArray, int> serializeBuffer(QSharedMemory *buffer) const;

    PendingItemIds pendingItemsInvalidated() const
    {
        Q_ASSERT_X(m_currentCacheId.isEmpty(), Q_FUNC_INFO,
                   "Call endSharedBufferAction() before accessing data");
        return m_pendingInvalidatedItems;
    }

    PendingItemIds pendingItemsReady() const
    {
        Q_ASSERT_X(m_currentCacheId.isEmpty(), Q_FUNC_INFO,
                   "Call endSharedBufferAction() before accessing data");
        return m_pendingReadyItems;
    }

    PendingItemIds pendingItemsMissing() const
    {
        Q_ASSERT_X(m_currentCacheId.isEmpty(), Q_FUNC_INFO,
                   "Call endSharedBufferAction() before accessing data");
        return m_pendingMissingItems;
    }

private:
    struct Item {
        Item()
            : next(0)
            , prev(0)
            , buffer(0)
            , itemId(0)
            , x(-1)
            , y(-1)
            , width(-1)
            , height(-1)
        {
        }

        Item *next;
        Item *prev;

        Buffer *buffer;
        quint32 itemId;
        int x;
        int y;
        int width;
        int height;
    };

    struct Items
    {
        Items() : leastRecentlyUsed(0), mostRecentlyUsed(0) {}

        Item *leastRecentlyUsed;
        Item *mostRecentlyUsed;

        QByteArray cacheId;
        QHash<quint32, Item *> items;
    };

    void findAvailableBuffer(int itemWidth, int itemHeight, Buffer **buffer, int *x, int *y);
    void recycleItem(Buffer **buffer, int *x, int *y);
    void copyIntoBuffer(Buffer *buffer, int x, int y, int itemWidth, int itemHeight, uchar *data);
    void touchBuffer(Buffer *buffer);
    void deleteLeastRecentlyUsed();

    Buffer *createNewBuffer(const QByteArray &cacheId, int heightRequired);
    Buffer *resizeBuffer(Buffer *buffer, const QSize &newSize);
    Buffer *allocateBuffer(int width, int height);

    Items *itemsForCache(const QByteArray &cacheId) const;
    void pushItemToBack(Items *items, Item *item);
    void touchItem(Items *items, Item *item);
    void deleteItem(Items *items, Item *item);
    void recycleItem(const QByteArray &cacheId, Buffer **sharedBuffer, int *glyphX, int *glyphY);

    QByteArray m_currentCacheId;

    quint32 m_memoryUsed;
    Buffer *m_mostRecentlyUsed;
    Buffer *m_leastRecentlyUsed;

    mutable QHash<QByteArray, Items *> m_items;
    QMultiHash<QByteArray, Buffer *> m_buffers;

    PendingItemIds m_pendingInvalidatedItems;
    PendingItemIds m_pendingReadyItems;
    PendingItemIds m_pendingMissingItems;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_USE_XCB_SHARED_GRAPHICS_CACHE

#endif // XCBSHAREDBUFFERMANAGER_H
