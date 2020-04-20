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

#include "qcocoanativeinterface.h"
#include "qcocoawindow.h"
#include "qcocoamenu.h"
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

#if !defined(QT_NO_WIDGETS) && defined(QT_PRINTSUPPORT_LIB)
#include "qcocoaprintersupport.h"
#include "qprintengine_mac_p.h"
#include <qpa/qplatformprintersupport.h>
#endif

#include <QtGui/private/qcoregraphics_p.h>

#include <QtPlatformHeaders/qcocoawindowfunctions.h>

#include <AppKit/AppKit.h>

#if QT_CONFIG(vulkan)
#include <MoltenVK/mvk_vulkan.h>
#endif

QT_BEGIN_NAMESPACE

QCocoaNativeInterface::QCocoaNativeInterface()
{
}

#ifndef QT_NO_OPENGL
void *QCocoaNativeInterface::nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context)
{
    if (!context)
        return nullptr;
    if (resourceString.toLower() == "nsopenglcontext")
        return nsOpenGLContextForContext(context);
    if (resourceString.toLower() == "cglcontextobj")
        return cglContextForContext(context);

    return nullptr;
}
#endif

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
    if (resource.toLower() == "addtomimelist")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::addToMimeList);
    if (resource.toLower() == "removefrommimelist")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::removeFromMimeList);
    if (resource.toLower() == "registerdraggedtypes")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::registerDraggedTypes);
    if (resource.toLower() == "setdockmenu")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setDockMenu);
    if (resource.toLower() == "qmenutonsmenu")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::qMenuToNSMenu);
    if (resource.toLower() == "qmenubartonsmenu")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::qMenuBarToNSMenu);
    if (resource.toLower() == "qimagetocgimage")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::qImageToCGImage);
    if (resource.toLower() == "cgimagetoqimage")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::cgImageToQImage);
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
    if (resource.toLower() == "setcontentborderenabled")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setContentBorderEnabled);
    if (resource.toLower() == "setnstoolbar")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::setNSToolbar);
    if (resource.toLower() == "testcontentborderposition")
        return NativeResourceForIntegrationFunction(QCocoaNativeInterface::testContentBorderPosition);

    return nullptr;
}

QPlatformPrinterSupport *QCocoaNativeInterface::createPlatformPrinterSupport()
{
#if !defined(QT_NO_WIDGETS) && !defined(QT_NO_PRINTER) && defined(QT_PRINTSUPPORT_LIB)
    return new QCocoaPrinterSupport();
#else
    qFatal("Printing is not supported when Qt is configured with -no-widgets or -no-feature-printer");
    return nullptr;
#endif
}

void *QCocoaNativeInterface::NSPrintInfoForPrintEngine(QPrintEngine *printEngine)
{
#if !defined(QT_NO_WIDGETS) && !defined(QT_NO_PRINTER) && defined(QT_PRINTSUPPORT_LIB)
    QMacPrintEnginePrivate *macPrintEnginePriv = static_cast<QMacPrintEngine *>(printEngine)->d_func();
    if (macPrintEnginePriv->state == QPrinter::Idle && !macPrintEnginePriv->isPrintSessionInitialized())
        macPrintEnginePriv->initialize();
    return macPrintEnginePriv->printInfo;
#else
    Q_UNUSED(printEngine);
    qFatal("Printing is not supported when Qt is configured with -no-widgets or -no-feature-printer");
    return nullptr;
#endif
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

#ifndef QT_NO_OPENGL
void *QCocoaNativeInterface::cglContextForContext(QOpenGLContext* context)
{
    NSOpenGLContext *nsOpenGLContext = static_cast<NSOpenGLContext*>(nsOpenGLContextForContext(context));
    if (nsOpenGLContext)
        return [nsOpenGLContext CGLContextObj];
    return nullptr;
}

void *QCocoaNativeInterface::nsOpenGLContextForContext(QOpenGLContext* context)
{
    if (context) {
        if (QCocoaGLContext *cocoaGLContext = static_cast<QCocoaGLContext *>(context->handle()))
            return cocoaGLContext->nativeContext();
    }
    return nullptr;
}
#endif

QFunctionPointer QCocoaNativeInterface::platformFunction(const QByteArray &function) const
{
    if (function == QCocoaWindowFunctions::bottomLeftClippedByNSWindowOffsetIdentifier())
        return QFunctionPointer(QCocoaWindowFunctions::BottomLeftClippedByNSWindowOffset(QCocoaWindow::bottomLeftClippedByNSWindowOffsetStatic));

    return nullptr;
}

void QCocoaNativeInterface::addToMimeList(void *macPasteboardMime)
{
    qt_mac_addToGlobalMimeList(reinterpret_cast<QMacInternalPasteboardMime *>(macPasteboardMime));
}

void QCocoaNativeInterface::removeFromMimeList(void *macPasteboardMime)
{
    qt_mac_removeFromGlobalMimeList(reinterpret_cast<QMacInternalPasteboardMime *>(macPasteboardMime));
}

void QCocoaNativeInterface::registerDraggedTypes(const QStringList &types)
{
    qt_mac_registerDraggedTypes(types);
}

void QCocoaNativeInterface::setDockMenu(QPlatformMenu *platformMenu)
{
    QMacAutoReleasePool pool;
    QCocoaMenu *cocoaPlatformMenu = static_cast<QCocoaMenu *>(platformMenu);
    NSMenu *menu = cocoaPlatformMenu->nsMenu();
    [QCocoaApplicationDelegate sharedDelegate].dockMenu = menu;
}

void *QCocoaNativeInterface::qMenuToNSMenu(QPlatformMenu *platformMenu)
{
    QCocoaMenu *cocoaPlatformMenu = static_cast<QCocoaMenu *>(platformMenu);
    NSMenu *menu = cocoaPlatformMenu->nsMenu();
    return reinterpret_cast<void *>(menu);
}

void *QCocoaNativeInterface::qMenuBarToNSMenu(QPlatformMenuBar *platformMenuBar)
{
    QCocoaMenuBar *cocoaPlatformMenuBar = static_cast<QCocoaMenuBar *>(platformMenuBar);
    NSMenu *menu = cocoaPlatformMenuBar->nsMenu();
    return reinterpret_cast<void *>(menu);
}

CGImageRef QCocoaNativeInterface::qImageToCGImage(const QImage &image)
{
    return qt_mac_toCGImage(image);
}

QImage QCocoaNativeInterface::cgImageToQImage(CGImageRef image)
{
    return qt_mac_toQImage(image);
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

void QCocoaNativeInterface::setContentBorderEnabled(QWindow *window, bool enable)
{
    if (!window)
        return;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (cocoaWindow)
        cocoaWindow->setContentBorderEnabled(enable);
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
