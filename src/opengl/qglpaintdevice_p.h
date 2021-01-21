/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QGLPAINTDEVICE_P_H
#define QGLPAINTDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL module.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//


#include <qpaintdevice.h>
#include <QtOpenGL/qgl.h>


QT_BEGIN_NAMESPACE

class Q_OPENGL_EXPORT QGLPaintDevice : public QPaintDevice
{
public:
    QGLPaintDevice();
    virtual ~QGLPaintDevice();

    int devType() const override {return QInternal::OpenGL;}

    virtual void beginPaint();
    virtual void ensureActiveTarget();
    virtual void endPaint();

    virtual QGLContext* context() const = 0;
    virtual QGLFormat format() const;
    virtual QSize size() const = 0;
    virtual bool alphaRequested() const;
    virtual bool isFlipped() const;

    // returns the QGLPaintDevice for the given QPaintDevice
    static QGLPaintDevice* getDevice(QPaintDevice*);

protected:
    int metric(QPaintDevice::PaintDeviceMetric metric) const override;
    GLuint m_previousFBO;
    GLuint m_thisFBO;
};


// Wraps a QGLWidget
class QGLWidget;
class Q_OPENGL_EXPORT QGLWidgetGLPaintDevice : public QGLPaintDevice
{
public:
    QGLWidgetGLPaintDevice();

    virtual QPaintEngine* paintEngine() const override;

    // QGLWidgets need to do swapBufers in endPaint:
    virtual void beginPaint() override;
    virtual void endPaint() override;
    virtual QSize size() const override;
    virtual QGLContext* context() const override;

    void setWidget(QGLWidget*);

private:
    friend class QGLWidget;
    QGLWidget *glWidget;
};

QT_END_NAMESPACE

#endif // QGLPAINTDEVICE_P_H
