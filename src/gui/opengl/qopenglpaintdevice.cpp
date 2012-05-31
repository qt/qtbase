/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qopenglpaintdevice.h>
#include <qpaintengine.h>
#include <qthreadstorage.h>

#include <private/qobject_p.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglframebufferobject_p.h>
#include <private/qopenglpaintengine_p.h>

// for qt_defaultDpiX/Y
#include <private/qfont_p.h>

#include <qopenglfunctions.h>

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLPaintDevice
    \brief The QOpenGLPaintDevice class enables painting to an OpenGL context using QPainter.
    \since 5.0

    \ingroup painting-3D

    When painting to a QOpenGLPaintDevice using QPainter, the state of
    the current OpenGL context will be altered by the paint engine to reflect
    its needs.  Applications should not rely upon the OpenGL state being reset
    to its original conditions, particularly the current shader program,
    OpenGL viewport, texture units, and drawing modes.
*/

class QOpenGLPaintDevicePrivate
{
public:
    QOpenGLPaintDevicePrivate(const QSize &size);

    QSize size;
    QOpenGLContext *ctx;

    qreal dpmx;
    qreal dpmy;

    bool flipped;

    QPaintEngine *engine;
};

/*!
    Constructs a QOpenGLPaintDevice.

    The QOpenGLPaintDevice is only valid for the current context.

    \sa QOpenGLContext::currentContext()
*/
QOpenGLPaintDevice::QOpenGLPaintDevice()
    : d_ptr(new QOpenGLPaintDevicePrivate(QSize()))
{
}

/*!
    Constructs a QOpenGLPaintDevice with the given \a size.

    The QOpenGLPaintDevice is only valid for the current context.

    \sa QOpenGLContext::currentContext()
*/
QOpenGLPaintDevice::QOpenGLPaintDevice(const QSize &size)
    : d_ptr(new QOpenGLPaintDevicePrivate(size))
{
}

/*!
    Constructs a QOpenGLPaintDevice with the given \a size and \a ctx.

    The QOpenGLPaintDevice is only valid for the current context.

    \sa QOpenGLContext::currentContext()
*/
QOpenGLPaintDevice::QOpenGLPaintDevice(int width, int height)
    : d_ptr(new QOpenGLPaintDevicePrivate(QSize(width, height)))
{
}

QOpenGLPaintDevice::~QOpenGLPaintDevice()
{
    delete d_ptr->engine;
}

QOpenGLPaintDevicePrivate::QOpenGLPaintDevicePrivate(const QSize &sz)
    : size(sz)
    , ctx(QOpenGLContext::currentContext())
    , dpmx(qt_defaultDpiX() * 100. / 2.54)
    , dpmy(qt_defaultDpiY() * 100. / 2.54)
    , flipped(false)
    , engine(0)
{
}

class QOpenGLEngineThreadStorage
{
public:
    QPaintEngine *engine() {
        QPaintEngine *&localEngine = storage.localData();
        if (!localEngine)
            localEngine = new QOpenGL2PaintEngineEx;
        return localEngine;
    }

private:
    QThreadStorage<QPaintEngine *> storage;
};

Q_GLOBAL_STATIC(QOpenGLEngineThreadStorage, qt_opengl_engine)

QPaintEngine *QOpenGLPaintDevice::paintEngine() const
{
    if (d_ptr->engine)
        return d_ptr->engine;

    QPaintEngine *engine = qt_opengl_engine()->engine();
    if (engine->isActive() && engine->paintDevice() != this) {
        d_ptr->engine = new QOpenGL2PaintEngineEx;
        return d_ptr->engine;
    }

    return engine;
}

QOpenGLContext *QOpenGLPaintDevice::context() const
{
    return d_ptr->ctx;
}

QSize QOpenGLPaintDevice::size() const
{
    return d_ptr->size;
}

void QOpenGLPaintDevice::setSize(const QSize &size)
{
    d_ptr->size = size;
}

int QOpenGLPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch (metric) {
    case PdmWidth:
        return d_ptr->size.width();
    case PdmHeight:
        return d_ptr->size.height();
    case PdmDepth:
        return 32;
    case PdmWidthMM:
        return qRound(d_ptr->size.width() * 1000 / d_ptr->dpmx);
    case PdmHeightMM:
        return qRound(d_ptr->size.height() * 1000 / d_ptr->dpmy);
    case PdmNumColors:
        return 0;
    case PdmDpiX:
        return qRound(d_ptr->dpmx * 0.0254);
    case PdmDpiY:
        return qRound(d_ptr->dpmy * 0.0254);
    case PdmPhysicalDpiX:
        return qRound(d_ptr->dpmx * 0.0254);
    case PdmPhysicalDpiY:
        return qRound(d_ptr->dpmy * 0.0254);
    default:
        qWarning("QOpenGLPaintDevice::metric() - metric %d not known", metric);
        return 0;
    }
}

qreal QOpenGLPaintDevice::dotsPerMeterX() const
{
    return d_ptr->dpmx;
}

qreal QOpenGLPaintDevice::dotsPerMeterY() const
{
    return d_ptr->dpmy;
}

void QOpenGLPaintDevice::setDotsPerMeterX(qreal dpmx)
{
    d_ptr->dpmx = dpmx;
}

void QOpenGLPaintDevice::setDotsPerMeterY(qreal dpmy)
{
    d_ptr->dpmx = dpmy;
}

/*!
    Specifies whether painting should be flipped around the Y-axis or not.
*/
void QOpenGLPaintDevice::setPaintFlipped(bool flipped)
{
    d_ptr->flipped = flipped;
}

bool QOpenGLPaintDevice::paintFlipped() const
{
    return d_ptr->flipped;
}

QT_END_NAMESPACE
