/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Qt
#include <private/qguiapplication_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>
#include <QtCore/QMap>

// Local
#include "qmirclientnativeinterface.h"
#include "qmirclientscreen.h"
#include "qmirclientglcontext.h"

class QMirClientResourceMap : public QMap<QByteArray, QMirClientNativeInterface::ResourceType>
{
public:
    QMirClientResourceMap()
        : QMap<QByteArray, QMirClientNativeInterface::ResourceType>() {
        insert("egldisplay", QMirClientNativeInterface::EglDisplay);
        insert("eglcontext", QMirClientNativeInterface::EglContext);
        insert("nativeorientation", QMirClientNativeInterface::NativeOrientation);
        insert("display", QMirClientNativeInterface::Display);
    }
};

Q_GLOBAL_STATIC(QMirClientResourceMap, ubuntuResourceMap)

QMirClientNativeInterface::QMirClientNativeInterface()
    : mGenericEventFilterType(QByteArrayLiteral("Event"))
    , mNativeOrientation(nullptr)
{
}

QMirClientNativeInterface::~QMirClientNativeInterface()
{
    delete mNativeOrientation;
    mNativeOrientation = nullptr;
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
