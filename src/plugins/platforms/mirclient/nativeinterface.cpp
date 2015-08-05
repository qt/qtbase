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
#include "nativeinterface.h"
#include "screen.h"
#include "glcontext.h"

class UbuntuResourceMap : public QMap<QByteArray, UbuntuNativeInterface::ResourceType>
{
public:
    UbuntuResourceMap()
        : QMap<QByteArray, UbuntuNativeInterface::ResourceType>() {
        insert("egldisplay", UbuntuNativeInterface::EglDisplay);
        insert("eglcontext", UbuntuNativeInterface::EglContext);
        insert("nativeorientation", UbuntuNativeInterface::NativeOrientation);
        insert("display", UbuntuNativeInterface::Display);
    }
};

Q_GLOBAL_STATIC(UbuntuResourceMap, ubuntuResourceMap)

UbuntuNativeInterface::UbuntuNativeInterface()
    : mGenericEventFilterType(QByteArrayLiteral("Event"))
    , mNativeOrientation(nullptr)
{
}

UbuntuNativeInterface::~UbuntuNativeInterface()
{
    delete mNativeOrientation;
    mNativeOrientation = nullptr;
}

void* UbuntuNativeInterface::nativeResourceForContext(
    const QByteArray& resourceString, QOpenGLContext* context)
{
    if (!context)
        return nullptr;

    const QByteArray kLowerCaseResource = resourceString.toLower();

    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return nullptr;

    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);

    if (kResourceType == UbuntuNativeInterface::EglContext)
        return static_cast<UbuntuOpenGLContext*>(context->handle())->eglContext();
    else
        return nullptr;
}

void* UbuntuNativeInterface::nativeResourceForWindow(const QByteArray& resourceString, QWindow* window)
{
    const QByteArray kLowerCaseResource = resourceString.toLower();
    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return NULL;
    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);
    if (kResourceType == UbuntuNativeInterface::EglDisplay) {
        if (window) {
            return static_cast<UbuntuScreen*>(window->screen()->handle())->eglDisplay();
        } else {
            return static_cast<UbuntuScreen*>(
                    QGuiApplication::primaryScreen()->handle())->eglDisplay();
        }
    } else if (kResourceType == UbuntuNativeInterface::NativeOrientation) {
        // Return the device's native screen orientation.
        if (window) {
            UbuntuScreen *ubuntuScreen = static_cast<UbuntuScreen*>(window->screen()->handle());
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

void* UbuntuNativeInterface::nativeResourceForScreen(const QByteArray& resourceString, QScreen* screen)
{
    const QByteArray kLowerCaseResource = resourceString.toLower();
    if (!ubuntuResourceMap()->contains(kLowerCaseResource))
        return NULL;
    const ResourceType kResourceType = ubuntuResourceMap()->value(kLowerCaseResource);
    if (kResourceType == UbuntuNativeInterface::Display) {
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        return static_cast<UbuntuScreen*>(screen->handle())->eglNativeDisplay();
    } else
        return NULL;
}
