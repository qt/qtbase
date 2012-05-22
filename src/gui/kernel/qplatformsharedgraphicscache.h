/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPLATFORMSHAREDGRAPHICSCACHE_H
#define QPLATFORMSHAREDGRAPHICSCACHE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qobject.h>
#include <QtGui/qimage.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QPlatformSharedGraphicsCache: public QObject
{
    Q_OBJECT
public:
    enum PixelFormat
    {
        Alpha8
    };

    enum BufferType
    {
        OpenGLTexture
    };

    explicit QPlatformSharedGraphicsCache(QObject *parent = 0) : QObject(parent) {}

    Q_INVOKABLE virtual void ensureCacheInitialized(const QByteArray &cacheId, BufferType bufferType,
                                                    PixelFormat pixelFormat) = 0;

    Q_INVOKABLE virtual void requestItems(const QByteArray &cacheId, const QVector<quint32> &itemIds) = 0;
    Q_INVOKABLE virtual void insertItems(const QByteArray &cacheId,
                                         const QVector<quint32> &itemIds,
                                         const QVector<QImage> &items) = 0;
    Q_INVOKABLE virtual void releaseItems(const QByteArray &cacheId, const QVector<quint32> &itemIds) = 0;

    virtual uint textureIdForBuffer(void *bufferId) = 0;
    virtual void referenceBuffer(void *bufferId) = 0;
    virtual bool dereferenceBuffer(void *bufferId) = 0;
    virtual QSize sizeOfBuffer(void *bufferId) = 0;
    virtual void *eglImageForBuffer(void *bufferId) = 0;

Q_SIGNALS:
    void itemsMissing(const QByteArray &cacheId, const QVector<quint32> &itemIds);
    void itemsAvailable(const QByteArray &cacheId, void *bufferId,
                        const QVector<quint32> &itemIds, const QVector<QPoint> &positionsInBuffer);
    void itemsInvalidated(const QByteArray &cacheId, const QVector<quint32> &itemIds);
    void itemsUpdated(const QByteArray &cacheId, void *bufferId,
                      const QVector<quint32> &itemIds, const QVector<QPoint> &positionsInBuffer);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMSHAREDGRAPHICSCACHE_H
