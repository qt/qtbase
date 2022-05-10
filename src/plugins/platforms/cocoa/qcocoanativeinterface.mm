// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

QPixmap QCocoaNativeInterface::defaultBackgroundPixmapForQWizard()
{
    // Note: starting with macOS 10.14, the KeyboardSetupAssistant app bundle no
    // longer contains the "Background.png" image. This function then returns a
    // null pixmap.
    const int ExpectedImageWidth = 242;
    const int ExpectedImageHeight = 414;
    QCFType<CFArrayRef> urls = LSCopyApplicationURLsForBundleIdentifier(
        CFSTR("com.apple.KeyboardSetupAssistant"), nullptr);
    if (urls && CFArrayGetCount(urls) > 0) {
        CFURLRef url = (CFURLRef)CFArrayGetValueAtIndex(urls, 0);
        QCFType<CFBundleRef> bundle = CFBundleCreate(kCFAllocatorDefault, url);
        if (bundle) {
            url = CFBundleCopyResourceURL(bundle, CFSTR("Background"), CFSTR("png"), nullptr);
            if (url) {
                QCFType<CGImageSourceRef> imageSource = CGImageSourceCreateWithURL(url, nullptr);
                QCFType<CGImageRef> image = CGImageSourceCreateImageAtIndex(imageSource, 0, nullptr);
                if (image) {
                    int width = CGImageGetWidth(image);
                    int height = CGImageGetHeight(image);
                    if (width == ExpectedImageWidth && height == ExpectedImageHeight)
                        return QPixmap::fromImage(qt_mac_toQImage(image));
                }
            }
        }
    }
    return QPixmap();
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
