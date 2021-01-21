/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QOPENGLPAINTDEVICE_H
#define QOPENGLPAINTDEVICE_H

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qpaintdevice.h>
#include <QtGui/qopengl.h>
#include <QtGui/qopenglcontext.h>

QT_BEGIN_NAMESPACE

class QOpenGLPaintDevicePrivate;

class Q_GUI_EXPORT QOpenGLPaintDevice : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QOpenGLPaintDevice)
public:
    QOpenGLPaintDevice();
    explicit QOpenGLPaintDevice(const QSize &size);
    QOpenGLPaintDevice(int width, int height);
    ~QOpenGLPaintDevice();

    int devType() const override { return QInternal::OpenGL; }
    QPaintEngine *paintEngine() const override;

    QOpenGLContext *context() const;
    QSize size() const;
    void setSize(const QSize &size);
    void setDevicePixelRatio(qreal devicePixelRatio);

    qreal dotsPerMeterX() const;
    qreal dotsPerMeterY() const;

    void setDotsPerMeterX(qreal);
    void setDotsPerMeterY(qreal);

    void setPaintFlipped(bool flipped);
    bool paintFlipped() const;

    virtual void ensureActiveTarget();

protected:
    QOpenGLPaintDevice(QOpenGLPaintDevicePrivate &dd);
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;

    Q_DISABLE_COPY(QOpenGLPaintDevice)
    QScopedPointer<QOpenGLPaintDevicePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLPAINTDEVICE_H
