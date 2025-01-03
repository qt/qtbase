// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSCONTEXT_H
#define QIOSCONTEXT_H

#include <QtCore/qloggingcategory.h>
#include <qpa/qplatformopenglcontext.h>

@class EAGLContext;

QT_BEGIN_NAMESPACE

class QIOSWindow;

class QIOSContext : public QObject, public QPlatformOpenGLContext
{
    Q_OBJECT

public:
    QIOSContext(QOpenGLContext *context);
    ~QIOSContext();

    QSurfaceFormat format() const override;

    void swapBuffers(QPlatformSurface *surface) override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    GLuint defaultFramebufferObject(QPlatformSurface *) const override;
    QFunctionPointer getProcAddress(const char *procName) override;

    bool isSharing() const override;
    bool isValid() const override;

private Q_SLOTS:
    void windowDestroyed(QObject *object);

private:
    QIOSContext *m_sharedContext;
    EAGLContext *m_eaglContext;
    QSurfaceFormat m_format;

    struct FramebufferObject {
        GLuint handle;
        GLuint colorRenderbuffer;
        GLuint depthRenderbuffer;
        GLint renderbufferWidth;
        GLint renderbufferHeight;
        bool isComplete;
    };

    static bool verifyGraphicsHardwareAvailability();
    static void deleteBuffers(const FramebufferObject &framebufferObject);

    FramebufferObject &backingFramebufferObjectFor(QPlatformSurface *) const;
    mutable QHash<QPlatformSurface *, FramebufferObject> m_framebufferObjects;

    bool needsRenderbufferResize(QPlatformSurface *) const;
};

QT_END_NAMESPACE

#endif // QIOSCONTEXT_H
