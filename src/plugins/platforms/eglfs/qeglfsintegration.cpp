/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfsintegration.h"

#include "qeglfswindow.h"
#include "qeglfshooks.h"
#include "qeglfscontext.h"
#include "qeglfsoffscreenwindow.h"

#include <QtGui/private/qguiapplication_p.h>

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglpbuffer_p.h>
#include <QtPlatformHeaders/QEGLNativeContext>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#include <qpa/qplatformcursor.h>

#include <EGL/egl.h>

static void initResources()
{
    Q_INIT_RESOURCE(cursor);
}

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
{
    mDisableInputHandlers = qEnvironmentVariableIntValue("QT_QPA_EGLFS_DISABLE_INPUT");

    initResources();
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (qt_egl_device_integration()->hasCapability(cap))
        return true;

    return QEGLPlatformIntegration::hasCapability(cap);
}

void QEglFSIntegration::addScreen(QPlatformScreen *screen)
{
    screenAdded(screen);
}

void QEglFSIntegration::initialize()
{
    qt_egl_device_integration()->platformInit();

    QEGLPlatformIntegration::initialize();

    if (!mDisableInputHandlers)
        createInputHandlers();

    if (qt_egl_device_integration()->usesDefaultScreen())
        addScreen(new QEglFSScreen(display()));
    else
        qt_egl_device_integration()->screenInit();
}

void QEglFSIntegration::destroy()
{
    qt_egl_device_integration()->screenDestroy();
    QEGLPlatformIntegration::destroy();
    qt_egl_device_integration()->platformDestroy();
}

EGLNativeDisplayType QEglFSIntegration::nativeDisplay() const
{
    return qt_egl_device_integration()->platformDisplay();
}

QEGLPlatformWindow *QEglFSIntegration::createWindow(QWindow *window) const
{
    return new QEglFSWindow(window);
}

QEGLPlatformContext *QEglFSIntegration::createContext(const QSurfaceFormat &format,
                                                      QPlatformOpenGLContext *shareContext,
                                                      EGLDisplay display,
                                                      QVariant *nativeHandle) const
{
    QEglFSContext *ctx;
    QSurfaceFormat adjustedFormat = qt_egl_device_integration()->surfaceFormatFor(format);
    if (!nativeHandle || nativeHandle->isNull()) {
        EGLConfig config = QEglFSIntegration::chooseConfig(display, adjustedFormat);
        ctx = new QEglFSContext(adjustedFormat, shareContext, display, &config, QVariant());
    } else {
        ctx = new QEglFSContext(adjustedFormat, shareContext, display, 0, *nativeHandle);
    }
    *nativeHandle = QVariant::fromValue<QEGLNativeContext>(QEGLNativeContext(ctx->eglContext(), display));
    return ctx;
}

QPlatformOffscreenSurface *QEglFSIntegration::createOffscreenSurface(EGLDisplay display,
                                                                     const QSurfaceFormat &format,
                                                                     QOffscreenSurface *surface) const
{
    QSurfaceFormat fmt = qt_egl_device_integration()->surfaceFormatFor(format);
    if (qt_egl_device_integration()->supportsPBuffers())
        return new QEGLPbuffer(display, fmt, surface);
    else
        return new QEglFSOffscreenWindow(display, fmt, surface);

    // Never return null. Multiple QWindows are not supported by this plugin.
}

EGLConfig QEglFSIntegration::chooseConfig(EGLDisplay display, const QSurfaceFormat &format)
{
    class Chooser : public QEglConfigChooser {
    public:
        Chooser(EGLDisplay display)
            : QEglConfigChooser(display) { }
        bool filterConfig(EGLConfig config) const Q_DECL_OVERRIDE {
            return qt_egl_device_integration()->filterConfig(display(), config)
                    && QEglConfigChooser::filterConfig(config);
        }
    };

    Chooser chooser(display);
    chooser.setSurfaceFormat(format);
    return chooser.chooseConfig();
}

QT_END_NAMESPACE
