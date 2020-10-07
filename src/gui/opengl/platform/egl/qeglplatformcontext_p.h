/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLPLATFORMCONTEXT_H
#define QEGLPLATFORMCONTEXT_H

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

#include <QtCore/qtextstream.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformopenglcontext.h>
#include <QtCore/qvariant.h>
#include <QtGui/private/qt_egl_p.h>
#include <QtGui/private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QEGLPlatformContext : public QPlatformOpenGLContext,
                                            public QNativeInterface::QEGLContext
{
public:
    enum Flag {
        NoSurfaceless = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                        EGLConfig *config = nullptr, Flags flags = { });

    template <typename T>
    static QOpenGLContext *createFrom(EGLContext context, EGLDisplay contextDisplay,
            EGLDisplay platformDisplay, QOpenGLContext *shareContext)
    {
        if (!context)
            return nullptr;

        // A context belonging to a given EGLDisplay cannot be used with another one
        if (contextDisplay != platformDisplay) {
            qWarning("QEGLPlatformContext: Cannot adopt context from different display");
            return nullptr;
        }

        QPlatformOpenGLContext *shareHandle = shareContext ? shareContext->handle() : nullptr;

        auto *resultingContext = new QOpenGLContext;
        auto *contextPrivate = QOpenGLContextPrivate::get(resultingContext);
        auto *platformContext = new T;
        platformContext->adopt(context, contextDisplay, shareHandle);
        contextPrivate->adopt(platformContext);
        return resultingContext;
    }

    ~QEGLPlatformContext();

    void initialize() override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override;
    bool isSharing() const override { return m_shareContext != EGL_NO_CONTEXT; }
    bool isValid() const override { return m_eglContext != EGL_NO_CONTEXT; }

    EGLContext nativeContext() const override { return eglContext(); }

    EGLContext eglContext() const;
    EGLDisplay eglDisplay() const;
    EGLConfig eglConfig() const;

protected:
    QEGLPlatformContext() {} // For adoption
    virtual EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) = 0;
    virtual EGLSurface createTemporaryOffscreenSurface();
    virtual void destroyTemporaryOffscreenSurface(EGLSurface surface);
    virtual void runGLChecks();

private:
    void adopt(EGLContext context, EGLDisplay display, QPlatformOpenGLContext *shareContext);
    void updateFormatFromGL();

    EGLContext m_eglContext;
    EGLContext m_shareContext;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    QSurfaceFormat m_format;
    EGLenum m_api;
    int m_swapInterval = -1;
    bool m_swapIntervalEnvChecked = false;
    int m_swapIntervalFromEnv = -1;
    Flags m_flags;
    bool m_ownsContext = false;
    QList<EGLint> m_contextAttrs;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QEGLPlatformContext::Flags)

QT_END_NAMESPACE

#endif //QEGLPLATFORMCONTEXT_H
