/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qeglfsbackingstore.h"

#ifndef QT_NO_WIDGETS
#include <QtOpenGL/private/qgl_p.h>
#include <QtOpenGL/private/qglpaintdevice_p.h>
#endif //QT_NO_WIDGETS

#include <QtGui/QPlatformOpenGLContext>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_WIDGETS
class QEglFSPaintDevice : public QGLPaintDevice
{
public:
    QEglFSPaintDevice(QEglFSScreen *screen)
        :QGLPaintDevice(), m_screen(screen)
    {
    #ifdef QEGL_EXTRA_DEBUG
        qWarning("QEglPaintDevice %p, %p",this, screen);
    #endif
    }

    QSize size() const { return m_screen->geometry().size(); }
    QGLContext* context() const { return QGLContext::fromOpenGLContext(m_screen->platformContext()->context()); }

    QPaintEngine *paintEngine() const { return qt_qgl_paint_engine(); }

    void  beginPaint(){
        QGLPaintDevice::beginPaint();
    }
private:
    QEglFSScreen *m_screen;
    QGLContext *m_context;
};
#endif //QT_NO_WIDGETS

QEglFSBackingStore::QEglFSBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_paintDevice(0)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore %p, %p", window, window->screen());
#endif
#ifdef QT_NO_WIDGETS
    m_paintDevice = new QImage(0,0);
#else
    m_paintDevice = new QEglFSPaintDevice(static_cast<QEglFSScreen *>(window->screen()->handle()));
#endif //QT_NO_WIDGETS
}

void QEglFSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore::flush %p", window);
#endif
#ifndef QT_NO_WIDGETS
    static_cast<QEglFSPaintDevice *>(m_paintDevice)->context()->swapBuffers();
#endif //QT_NO_WIDGETS
}

void QEglFSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
}

QT_END_NAMESPACE
