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
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::registerDraggedTypes);
    if (resource.toLower() == "registertouchwindow")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::registerTouchWindow);
    if (resource.toLower() == "setembeddedinforeignview")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setEmbeddedInForeignView);
    if (resource.toLower() == "setcontentborderthickness")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setContentBorderThickness);
    if (resource.toLower() == "registercontentborderarea")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::registerContentBorderArea);
    if (resource.toLower() == "setcontentborderareaenabled")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setContentBorderAreaEnabled);
    if (resource.toLower() == "setnstoolbar")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setNSToolbar);
    if (resource.toLower() == "testcontentborderposition")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::testContentBorderPosition);

    return nullptr;
}

void QCocoaNativeInterface::clearCurrentThreadCocoaEventDispatcherInterruptFlag()
{
    QCocoaEventDispatcher::clearCurrentThreadCocoaEventDispatcherInterruptFlag();
}

void QCocoaNativeInterface::onAppFocusWindowChanged(QWindow *window)
{
    Q_UNUSED(window);
    QCocoaMenuBar::updateMenuBarImmediately();
}

void QCocoaNativeInterface::registerDraggedTypes(const QStringList &types)
{
    qt_mac_registerDraggedTypes(types);
}

void QCocoaNativeInterface::setEmbeddedInForeignView(QPlatformWindow *window, bool embedded)
{
    Q_UNUSED(embedded); // "embedded" state is now automatically detected
    QCocoaWindow *cocoaPlatformWindow = static_cast<QCocoaWindow *>(window);
    cocoaPlatformWindow->setEmbeddedInForeignView();
}

void QCocoaNativeInterface::registerTouchWindow(QWindow *window,  bool enable)
{
    if (!window)
        return;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->registerTouch(enable);
}

void QCocoaNativeInterface::setContentBorderThickness(QWindow *window, int topThickness, int bottomThickness)
{
    if (!window)
        return;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->setContentBorderThickness(topThickness, bottomThickness);
}

void QCocoaNativeInterface::registerContentBorderArea(QWindow *window, quintptr identifier, int upper, int lower)
{
    if (!window)
        return;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->registerContentBorderArea(identifier, upper, lower);
}

void QCocoaNativeInterface::setContentBorderAreaEnabled(QWindow *window, quintptr identifier, bool enable)
{
    if (!window)
        return;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->setContentBorderAreaEnabled(identifier, enable);
}

void QCocoaNativeInterface::setNSToolbar(QWindow *window, void *nsToolbar)
{
    QCocoaIntegration::instance()->setToolbar(window, static_cast<NSToolbar *>(nsToolbar));

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->updateNSToolbar();
}

bool QCocoaNativeInterface::testContentBorderPosition(QWindow *window, int position)
{
    if (!window)
        return false;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        return cocoaWindow->testContentBorderAreaPosition(position);
    return false;
}

QT_END_NAMESPACE
