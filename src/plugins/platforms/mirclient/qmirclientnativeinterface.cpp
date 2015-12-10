/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


// Local
#include "qmirclientnativeinterface.h"
#include "qmirclientscreen.h"
#include "qmirclientglcontext.h"

// Qt
#include <private/qguiapplication_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>
#include <QtCore/QMap>

class QMirClientResourceMap : public QMap<QByteArray, QMirClientNativeInterface::ResourceType>
{
public:
    QMirClientResourceMap()
        : QMap<QByteArray, QMirClientNativeInterface::ResourceType>() {
        insert("egldisplay", QMirClientNativeInterface::EglDisplay);
        insert("eglcontext", QMirClientNativeInterface::EglContext);
        insert("nativeorientation", QMirClientNativeInterface::NativeOrientation);
        insert("display", QMirClientNativeInterface::Display);
        insert("mirconnection", QMirClientNativeInterface::MirConnection);
    }
};

Q_GLOBAL_STATIC(QMirClientResourceMap, ubuntuResourceMap)

QMirClientNativeInterface::QMirClientNativeInterface()
    : mGenericEventFilterType(QByteArrayLiteral("Event"))
    , mNativeOrientation(nullptr)
    , mMirConnection(nullptr)
{
}

QMirClientNativeInterface::~QMirClientNativeInterface()
{
    delete mNativeOrientation;
    mNativeOrientation = nullptr;
}

void* QMirClientNativeInterface::nativeResourceForIntegration(const QByteArray &resourceString)
{
    const QByteArray lowerCaseResource = resourceString.toLower();

    if (!ubuntuResourceMap()->contains(lowerCaseResource)) {
        return nullptr;
    }

    const ResourceType resourceType = ubuntuResourceMap()->value(lowerCaseResource);

    if (resourceType == QMirClientNativeInterface::MirConnection) {
        return mMirConnection;
    } else {
        return nullptr;
    }
}

void* QMirClientNativeInterface::nativeResourceForContext(
    const QByteArray& resourceString, QOpenGLContext* context)
{
    if (!context)
        return nullptr;

    const QByteArray kLowerCaseResource = resourceString.toLower();

    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return nullptr;

    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);

    if (kResourceType == QMirClientNativeInterface::EglContext)
        return static_cast<QMirClientOpenGLContext*>(context->handle())->eglContext();
    else
        return nullptr;
}

void* QMirClientNativeInterface::nativeResourceForWindow(const QByteArray& resourceString, QWindow* window)
{
    const QByteArray kLowerCaseResource = resourceString.toLower();
    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return NULL;
    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);
    if (kResourceType == QMirClientNativeInterface::EglDisplay) {
        if (window) {
            return static_cast<QMirClientScreen*>(window->screen()->handle())->eglDisplay();
        } else {
            return static_cast<QMirClientScreen*>(
                    QGuiApplication::primaryScreen()->handle())->eglDisplay();
        }
    } else if (kResourceType == QMirClientNativeInterface::NativeOrientation) {
        // Return the device's native screen orientation.
        if (window) {
            QMirClientScreen *ubuntuScreen = static_cast<QMirClientScreen*>(window->screen()->handle());
            mNativeOrientation = new Qt::ScreenOrientation(ubuntuScreen->nativeOrientation());
        } else {
            QPlatformScreen *platformScreen = QGuiApplication::primaryScreen()->handle();
            mNativeOrientation = new Qt::ScreenOrientation(platformScreen->nativeOrientation());
        }
        return mNativeOrientation;
    } else {
        return NULL;
    }
}

void* QMirClientNativeInterface::nativeResourceForScreen(const QByteArray& resourceString, QScreen* screen)
{
    const QByteArray kLowerCaseResource = resourceString.toLower();
    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return NULL;
    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);
    if (kResourceType == QMirClientNativeInterface::Display) {
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        return static_cast<QMirClientScreen*>(screen->handle())->eglNativeDisplay();
    } else
        return NULL;
}
