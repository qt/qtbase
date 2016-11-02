/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
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


// Local
#include "qmirclientnativeinterface.h"
#include "qmirclientscreen.h"
#include "qmirclientglcontext.h"
#include "qmirclientwindow.h"

// Qt
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>
#include <QtCore/QMap>

class UbuntuResourceMap : public QMap<QByteArray, QMirClientNativeInterface::ResourceType>
{
public:
    UbuntuResourceMap()
        : QMap<QByteArray, QMirClientNativeInterface::ResourceType>() {
        insert("egldisplay", QMirClientNativeInterface::EglDisplay);
        insert("eglcontext", QMirClientNativeInterface::EglContext);
        insert("nativeorientation", QMirClientNativeInterface::NativeOrientation);
        insert("display", QMirClientNativeInterface::Display);
        insert("mirconnection", QMirClientNativeInterface::MirConnection);
        insert("mirsurface", QMirClientNativeInterface::MirSurface);
        insert("scale", QMirClientNativeInterface::Scale);
        insert("formfactor", QMirClientNativeInterface::FormFactor);
    }
};

Q_GLOBAL_STATIC(UbuntuResourceMap, ubuntuResourceMap)

QMirClientNativeInterface::QMirClientNativeInterface(const QMirClientClientIntegration *integration)
    : mIntegration(integration)
    , mGenericEventFilterType(QByteArrayLiteral("Event"))
    , mNativeOrientation(nullptr)
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
        return mIntegration->mirConnection();
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

    switch (kResourceType) {
    case EglDisplay:
        return mIntegration->eglDisplay();
    case NativeOrientation:
        // Return the device's native screen orientation.
        if (window) {
            QMirClientScreen *ubuntuScreen = static_cast<QMirClientScreen*>(window->screen()->handle());
            mNativeOrientation = new Qt::ScreenOrientation(ubuntuScreen->nativeOrientation());
        } else {
            QPlatformScreen *platformScreen = QGuiApplication::primaryScreen()->handle();
            mNativeOrientation = new Qt::ScreenOrientation(platformScreen->nativeOrientation());
        }
        return mNativeOrientation;
    case MirSurface:
        if (window) {
            auto ubuntuWindow = static_cast<QMirClientWindow*>(window->handle());
            if (ubuntuWindow) {
                return ubuntuWindow->mirSurface();
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    default:
        return nullptr;
    }
}

void* QMirClientNativeInterface::nativeResourceForScreen(const QByteArray& resourceString, QScreen* screen)
{
    const QByteArray kLowerCaseResource = resourceString.toLower();
    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return NULL;
    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    auto ubuntuScreen = static_cast<QMirClientScreen*>(screen->handle());
    if (kResourceType == QMirClientNativeInterface::Display) {
        return mIntegration->eglNativeDisplay();
    // Changes to the following properties are emitted via the QMirClientNativeInterface::screenPropertyChanged
    // signal fired by QMirClientScreen. Connect to this signal for these properties updates.
    // WARNING: code highly thread unsafe!
    } else if (kResourceType == QMirClientNativeInterface::Scale) {
        // In application code, read with:
        //    float scale = *reinterpret_cast<float*>(nativeResourceForScreen("scale", screen()));
        return &ubuntuScreen->mScale;
    } else if (kResourceType == QMirClientNativeInterface::FormFactor) {
        return &ubuntuScreen->mFormFactor;
    } else
        return NULL;
}

// Changes to these properties are emitted via the QMirClientNativeInterface::windowPropertyChanged
// signal fired by QMirClientWindow. Connect to this signal for these properties updates.
QVariantMap QMirClientNativeInterface::windowProperties(QPlatformWindow *window) const
{
    QVariantMap propertyMap;
    auto w = static_cast<QMirClientWindow*>(window);
    if (w) {
        propertyMap.insert("scale", w->scale());
        propertyMap.insert("formFactor", w->formFactor());
    }
    return propertyMap;
}

QVariant QMirClientNativeInterface::windowProperty(QPlatformWindow *window, const QString &name) const
{
    auto w = static_cast<QMirClientWindow*>(window);
    if (!w) {
        return QVariant();
    }

    if (name == QStringLiteral("scale")) {
        return w->scale();
    } else if (name == QStringLiteral("formFactor")) {
        return w->formFactor();
    } else {
        return QVariant();
    }
}

QVariant QMirClientNativeInterface::windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const
{
    QVariant returnVal = windowProperty(window, name);
    if (!returnVal.isValid()) {
        return defaultValue;
    } else {
        return returnVal;
    }
}
