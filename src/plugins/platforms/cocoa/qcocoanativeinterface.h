/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOANATIVEINTERFACE_H
#define QCOCOANATIVEINTERFACE_H

#include <ApplicationServices/ApplicationServices.h>

#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QPlatformPrinterSupport;
class QPrintEngine;
class QPlatformMenu;
class QPlatformMenuBar;

class QCocoaNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
public:
    QCocoaNativeInterface();

    void *nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context);
    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window);

    NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) Q_DECL_OVERRIDE;

    Q_INVOKABLE void beep();

    static void *cglContextForContext(QOpenGLContext *context);
    static void *nsOpenGLContextForContext(QOpenGLContext* context);

public Q_SLOTS:
    void onAppFocusWindowChanged(QWindow *window);

private:
    /*
        "Virtual" function to create the platform printer support
        implementation.

        We use an invokable function instead of a virtual one, we do not want
        this in the QPlatform* API yet.

        This was added here only because QPlatformNativeInterface is a QObject
        and allow us to use QMetaObject::indexOfMethod() from the printsupport
        plugin.
    */
    Q_INVOKABLE QPlatformPrinterSupport *createPlatformPrinterSupport();
    /*
        Function to return the NSPrintInfo * from QMacPaintEnginePrivate.
        Needed by the native print dialog in the Qt Print Support module.
    */
    Q_INVOKABLE void *NSPrintInfoForPrintEngine(QPrintEngine *printEngine);

    // QMacPastebardMime support. The mac pasteboard void pointers are
    // QMacPastebardMime instances from the cocoa plugin or qtmacextras
    // These two classes are kept in sync and can be casted between.
    static void addToMimeList(void *macPasteboardMime);
    static void removeFromMimeList(void *macPasteboardMime);
    static void registerDraggedTypes(const QStringList &types);

    // Dock menu support
    static void setDockMenu(QPlatformMenu *platformMenu);

    // Function to return NSMenu * from QPlatformMenu
    static void *qMenuToNSMenu(QPlatformMenu *platformMenu);

    // Function to return NSMenu * from QPlatformMenuBar
    static void *qMenuBarToNSMenu(QPlatformMenuBar *platformMenuBar);

    // QImage <-> CGImage conversion functions
    static CGImageRef qImageToCGImage(const QImage &image);
    static QImage cgImageToQImage(CGImageRef image);

    // Embedding NSViews as child QWindows
    static void setWindowContentView(QPlatformWindow *window, void *nsViewContentView);

    // Set a QWindow as a "guest" (subwindow) of a non-QWindow
    static void setEmbeddedInForeignView(QPlatformWindow *window, bool embedded);

    // Register if a window should deliver touch events. Enabling
    // touch events has implications for delivery of other events,
    // for example by causing scrolling event lag.
    //
    // The registration is ref-counted: multiple widgets can enable
    // touch events, which then will be delivered until the widget
    // deregisters.
    static void registerTouchWindow(QWindow *window,  bool enable);
};

#endif // QCOCOANATIVEINTERFACE_H

QT_END_NAMESPACE
