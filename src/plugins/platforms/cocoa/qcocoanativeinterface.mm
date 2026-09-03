// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <AppKit/AppKit.h>

#include "qcocoanativeinterface.h"
#include "qcocoawindow.h"
#include "qcocoamenu.h"
#include "qcocoansmenu.h"
#include "qcocoamenubar.h"
#include "qcocoahelpers.h"
#include "qcocoaapplicationdelegate.h"
#include "qcocoaintegration.h"
#include "qcocoaeventdispatcher.h"

#include <qbytearray.h>
#include <qwindow.h>
#include <qpixmap.h>
#include <qpa/qplatformwindow.h>
#include <QtGui/qsurfaceformat.h>
#ifndef QT_NO_OPENGL
#include <qpa/qplatformopenglcontext.h>
#include <QtGui/qopenglcontext.h>
#include "qcocoaglcontext.h"
#endif
#include <QtGui/qguiapplication.h>
#include <qdebug.h>

#include <QtGui/private/qmacmimeregistry_p.h>
#include <QtGui/private/qcoregraphics_p.h>

#if QT_CONFIG(vulkan)
#include <MoltenVK/mvk_vulkan.h>
#endif

QT_BEGIN_NAMESPACE

QCocoaNativeInterface::QCocoaNativeInterface()
{
}

void *QCocoaNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    if (!window->handle())
        return nullptr;

    if (resourceString == "nsview") {
        return static_cast<QCocoaWindow *>(window->handle())->m_view;
    } else if (resourceString == "nswindow") {
        return static_cast<QCocoaWindow *>(window->handle())->nativeWindow();
#if QT_CONFIG(vulkan)
    } else if (resourceString == "vkSurface") {
        if (QVulkanInstance *instance = window->vulkanInstance())
            return static_cast<QCocoaVulkanInstance *>(instance->handle())->surface(window);
#endif
    }
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QCocoaNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
    if (resource.toLower() == "registerdraggedtypes")
        return NativeResourceForIntegrationFunction(QFunctionPointer(QCocoaNativeInterface::registerDraggedTypes));
    if (resource.toLower() == "registertouchwindow")
        return NativeResourceForIntegrationFunction(QFunctionPointer(QCocoaNativeInterface::registerTouchWindow));
    return nullptr;
}

void QCocoaNativeInterface::clearCurrentThreadCocoaEventDispatcherInterruptFlag()
{
    QCocoaEventDispatcher::clearCurrentThreadCocoaEventDispatcherInterruptFlag();
}

void QCocoaNativeInterface::registerDraggedTypes(const QStringList &types)
{
    QMacMimeRegistry::registerDraggedTypes(types);
}

void QCocoaNativeInterface::registerTouchWindow(QWindow *window,  bool enable)
{
    if (!window)
        return;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->registerTouch(enable);
}

QT_END_NAMESPACE
