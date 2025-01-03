// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsemulatorintegration.h"
#include "qeglfsemulatorscreen.h"
#include "private/qeglfsintegration_p.h"

#include <private/qguiapplication_p.h>
#include <QtGui/private/qeglconvenience_p.h>
#include <QtGui/private/qeglplatformcontext_p.h>

#include <qpa/qwindowsysteminterface.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonParseError>

QT_BEGIN_NAMESPACE

QEglFSEmulatorIntegration::QEglFSEmulatorIntegration()
{
    // The Qt Emulator provides the ability to render to multiple displays
    // In addition to the usual EGL and OpenGLESv2 API's, there are also a
    // few additional API's that enable the client (this plugin) to query
    // the available screens and their properties, as well as the ability
    // to select which screen is the active render target (as this is
    // usually handled in a platform specific way and not by EGL itself).

    getDisplays = reinterpret_cast<PFNQGSGETDISPLAYSPROC>(eglGetProcAddress("qgsGetDisplays"));
    setDisplay = reinterpret_cast<PFNQGSSETDISPLAYPROC>(eglGetProcAddress("qgsSetDisplay"));
}

void QEglFSEmulatorIntegration::platformInit()
{
}

void QEglFSEmulatorIntegration::platformDestroy()
{
}

bool QEglFSEmulatorIntegration::usesDefaultScreen()
{
    // This makes it possible to remotely query and then register our own set of screens
    return false;
}

void QEglFSEmulatorIntegration::screenInit()
{
    // Use qgsGetDisplays() call to retrieve the available screens from the Emulator
    if (getDisplays) {
        QByteArray displaysInfo = getDisplays();
        QJsonParseError error;
        QJsonDocument displaysDocument = QJsonDocument::fromJson(displaysInfo, &error);
        if (error.error == QJsonParseError::NoError) {
            // Document should be an array of screen objects
            if (displaysDocument.isArray()){
                QJsonArray screenArray = displaysDocument.array();
                for (auto screenValue : screenArray) {
                    if (screenValue.isObject())
                        QWindowSystemInterface::handleScreenAdded(new QEglFSEmulatorScreen(screenValue.toObject()));
                }
            }
        } else {
            qWarning() << "eglfs_emu: Failed to parse display info JSON with error: " << error.errorString()
                       << " at offset " << error.offset << " : " << displaysInfo;

        }
    } else {
        qFatal("EGL library doesn't support Emulator extensions");
    }
}

bool QEglFSEmulatorIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedPixmaps:
    case QPlatformIntegration::OpenGL:
    case QPlatformIntegration::ThreadedOpenGL:
        return true;
    default:
        return false;
    }
}

EGLNativeWindowType QEglFSEmulatorIntegration::createNativeWindow(QPlatformWindow *platformWindow,
                                                                  const QSize &size,
                                                                  const QSurfaceFormat &format)
{
    Q_UNUSED(size);
    Q_UNUSED(format);
    QEglFSEmulatorScreen *screen = static_cast<QEglFSEmulatorScreen *>(platformWindow->screen());
    if (screen && setDisplay) {
        // Let the emulator know which screen the window surface is attached to
        setDisplay(screen->id());
    }
    Q_CONSTINIT static QBasicAtomicInt uniqueWindowId = Q_BASIC_ATOMIC_INITIALIZER(0);
    return EGLNativeWindowType(qintptr(1 + uniqueWindowId.fetchAndAddRelaxed(1)));
}

QT_END_NAMESPACE
