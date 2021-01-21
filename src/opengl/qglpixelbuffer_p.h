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

#ifndef QGLPIXELBUFFER_P_H
#define QGLPIXELBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtOpenGL/qglpixelbuffer.h"
#include <private/qgl_p.h>
#include <private/qglpaintdevice_p.h>

QT_BEGIN_NAMESPACE

class QEglContext;
class QOpenGLFramebufferObject;

class QGLPBufferGLPaintDevice : public QGLPaintDevice
{
public:
    QPaintEngine* paintEngine() const override {return pbuf->paintEngine();}
    QSize size() const override {return pbuf->size();}
    QGLContext* context() const override;
    void beginPaint() override;
    void endPaint() override;
    void setPBuffer(QGLPixelBuffer* pb);
    void setFbo(GLuint fbo);
private:
    QGLPixelBuffer* pbuf;
};

class QGLPixelBufferPrivate {
    Q_DECLARE_PUBLIC(QGLPixelBuffer)
public:
    QGLPixelBufferPrivate(QGLPixelBuffer *q) : q_ptr(q), invalid(true), qctx(nullptr), widget(nullptr), fbo(nullptr), blit_fbo(nullptr), pbuf(nullptr), ctx(nullptr)
    {
    }
    bool init(const QSize &size, const QGLFormat &f, QGLWidget *shareWidget);
    void common_init(const QSize &size, const QGLFormat &f, QGLWidget *shareWidget);
    bool cleanup();

    QGLPixelBuffer *q_ptr;
    bool invalid;
    QGLContext *qctx;
    QGLPBufferGLPaintDevice glDevice;
    QGLWidget *widget;
    QOpenGLFramebufferObject *fbo;
    QOpenGLFramebufferObject *blit_fbo;
    QGLFormat format;

    QGLFormat req_format;
    QPointer<QGLWidget> req_shareWidget;
    QSize req_size;

    //stubs
    void *pbuf;
    void *ctx;
};

QT_END_NAMESPACE

#endif // QGLPIXELBUFFER_P_H
