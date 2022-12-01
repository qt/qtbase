// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QFBOPAINTDEVICE_H
#define QFBOPAINTDEVICE_H

#ifndef QT_NO_OPENGL

#include <QImage>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QSurface>

class QFboPaintDevice : public QOpenGLPaintDevice {
public:
    QFboPaintDevice(const QSize &size, bool flipped = false, bool clearOnInit = true,
        QOpenGLFramebufferObject::Attachment = QOpenGLFramebufferObject::CombinedDepthStencil);
    ~QFboPaintDevice();

    // QOpenGLPaintDevice:
    void ensureActiveTarget() override;

    bool isValid() const { return m_framebufferObject->isValid(); }
    GLuint handle() const { return m_framebufferObject->handle(); }
    GLuint texture();
    GLuint takeTexture();
    QImage toImage() const;

    bool bind() { return m_framebufferObject->bind(); }
    bool release() { return m_framebufferObject->release(); }
    QSize size() const { return m_framebufferObject->size(); }

    QOpenGLFramebufferObject* framebufferObject() { return m_framebufferObject; }
    const QOpenGLFramebufferObject* framebufferObject() const { return m_framebufferObject; }

    static bool isSupported() { return QOpenGLFramebufferObject::hasOpenGLFramebufferObjects(); }

private:
    QOpenGLFramebufferObject *m_framebufferObject;
    QOpenGLFramebufferObject *m_resolvedFbo;
    QSurface *m_surface;
};

#endif // QT_NO_OPENGL

#endif // QFBOPAINTDEVICE_H
