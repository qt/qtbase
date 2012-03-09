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

#ifndef QQNXGLBACKINGSTORE_H
#define QQNXGLBACKINGSTORE_H

#include <QtGui/qplatformbackingstore_qpa.h>
#include <QtOpenGL/private/qglpaintdevice_p.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QGLContext;
class QQnxGLContext;
class QQnxScreen;
class QQnxWindow;

class QQnxGLPaintDevice : public QGLPaintDevice
{
public:
    QQnxGLPaintDevice(QWindow *window);
    virtual ~QQnxGLPaintDevice();

    virtual QPaintEngine *paintEngine() const;
    virtual QSize size() const;
    virtual QGLContext *context() const { return m_glContext; }

private:
    QQnxWindow *m_window;
    QGLContext *m_glContext;
};

class QQnxGLBackingStore : public QPlatformBackingStore
{
public:
    QQnxGLBackingStore(QWindow *window);
    virtual ~QQnxGLBackingStore();

    virtual QPaintDevice *paintDevice() { return m_paintDevice; }
    virtual void flush(QWindow *window, const QRegion &region, const QPoint &offset);
    virtual void resize(const QSize &size, const QRegion &staticContents);
    virtual void beginPaint(const QRegion &region);
    virtual void endPaint(const QRegion &region);

    void resizeSurface(const QSize &size);

private:
    QOpenGLContext *m_openGLContext;
    QQnxGLPaintDevice *m_paintDevice;
    QSize m_requestedSize;
    QSize m_size;
};

QT_END_NAMESPACE

#endif // QQNXGLBACKINGSTORE_H
