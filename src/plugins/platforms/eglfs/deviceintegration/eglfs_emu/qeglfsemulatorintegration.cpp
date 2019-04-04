/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qeglfsemulatorintegration.h"
#include "qeglfsemulatorscreen.h"
#include "private/qeglfsintegration_p.h"

#include <private/qguiapplication_p.h>
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtEglSupport/private/qeglplatformcontext_p.h>

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
    static QBasicAtomicInt uniqueWindowId = Q_BASIC_ATOMIC_INITIALIZER(0);
    return EGLNativeWindowType(qintptr(1 + uniqueWindowId.fetchAndAddRelaxed(1)));
}

QT_END_NAMESPACE
