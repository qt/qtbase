/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#include "qbbrasterbackingstore.h"
#include "qbbwindow.h"

#include <QtCore/QDebug>

#include <errno.h>

QT_BEGIN_NAMESPACE

QBBRasterBackingStore::QBBRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
#if defined(QBBRASTERBACKINGSTORE_DEBUG)
    qDebug() << "QBBRasterBackingStore::QBBRasterBackingStore - w=" << window;
#endif

    // save platform window associated with widget
    m_platformWindow = static_cast<QBBWindow*>(window->handle());
}

QBBRasterBackingStore::~QBBRasterBackingStore()
{
#if defined(QBBRasterBackingStore_DEBUG)
    qDebug() << "QBBRasterBackingStore::~QBBRasterBackingStore - w=" << window();
#endif
}

QPaintDevice *QBBRasterBackingStore::paintDevice()
{
    return m_platformWindow->renderBuffer().image();
}

void QBBRasterBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(offset);

#if defined(QBBRASTERBACKINGSTORE_DEBUG)
    qDebug() << "QBBRasterBackingStore::flush - w=" << this->window();
#endif

    // visit all pending scroll operations
    for (int i = m_scrollOpList.size() - 1; i >= 0; i--) {

        // do the scroll operation
        ScrollOp &op = m_scrollOpList[i];
        QRegion srcArea = op.totalArea.intersected( op.totalArea.translated(-op.dx, -op.dy) );
        m_platformWindow->scroll(srcArea, op.dx, op.dy);
    }

    // clear all pending scroll operations
    m_scrollOpList.clear();

    // update the display with newly rendered content
    m_platformWindow->post(region);
}

void QBBRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
#if defined(QBBRASTERBACKINGSTORE_DEBUG)
    qDebug() << "QBBRasterBackingStore::resize - w=" << window() << ", s=" << size;
#endif

    // NOTE: defer resizing window buffers until next paint as
    // resize() can be called multiple times before a paint occurs
}

bool QBBRasterBackingStore::scroll(const QRegion &area, int dx, int dy)
{
#if defined(QBBRASTERBACKINGSTORE_DEBUG)
    qDebug() << "QBBRasterBackingStore::scroll - w=" << window();
#endif

    // calculate entire region affected by scroll operation (src + dst)
    QRegion totalArea = area.translated(dx, dy);
    totalArea += area;

    // visit all pending scroll operations
    for (int i = m_scrollOpList.size() - 1; i >= 0; i--) {

        ScrollOp &op = m_scrollOpList[i];
        if (op.totalArea == totalArea) {
            // same area is being scrolled again - update delta
            op.dx += dx;
            op.dy += dy;
            return true;
        } else if (op.totalArea.intersects(totalArea)) {
            // current scroll overlaps previous scroll but is
            // not equal in area - just paint everything
            qWarning("QBB: pending scroll operations overlap but not equal");
            return false;
        }
    }

    // create new scroll operation
    m_scrollOpList.append( ScrollOp(totalArea, dx, dy) );
    return true;
}

void QBBRasterBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

#if defined(QBBRASTERBACKINGSTORE_DEBUG)
    qDebug() << "QBBRasterBackingStore::beginPaint - w=" << window();
#endif

    // resize window buffers if surface resized
    QSize s = window()->size();
    if (s != m_platformWindow->bufferSize()) {
        m_platformWindow->setBufferSize(s);
    }
}

void QBBRasterBackingStore::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
#if defined(QBBRasterBackingStore_DEBUG)
    qDebug() << "QBBRasterBackingStore::endPaint - w=" << window();
#endif
}

QT_END_NAMESPACE
