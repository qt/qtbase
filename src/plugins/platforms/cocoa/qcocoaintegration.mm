/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoaintegration.h"

#include "qcocoawindow.h"
#include "qcocoabackingstore.h"
#include "qcocoanativeinterface.h"
#include "qcocoamenuloader.h"
#include "qcocoaeventdispatcher.h"
#include "qcocoahelpers.h"
#include "qcocoaapplication.h"
#include "qcocoaapplicationdelegate.h"
#include "qmenu_mac.h"

#include <QtGui/qplatformaccessibility_qpa.h>
#include <QtCore/qcoreapplication.h>

#include <QtPlatformSupport/private/qbasicfontdatabase_p.h>

QT_BEGIN_NAMESPACE

QCocoaScreen::QCocoaScreen(int screenIndex)
    :QPlatformScreen()
{
    m_screen = [[NSScreen screens] objectAtIndex:screenIndex];
    NSRect rect = [m_screen frame];
    m_geometry = QRect(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height);

    m_format = QImage::Format_ARGB32;

    m_depth = NSBitsPerPixelFromDepth([m_screen depth]);

    const int dpi = 72;
    const qreal inch = 25.4;
    m_physicalSize = QSizeF(m_geometry.size()) * inch / dpi;
}

QCocoaScreen::~QCocoaScreen()
{
}

QCocoaIntegration::QCocoaIntegration()
    : mFontDb(new QBasicFontDatabase())
    , mEventDispatcher(new QCocoaEventDispatcher())
{
    mPool = new QCocoaAutoReleasePool;

    qApp->setAttribute(Qt::AA_DontUseNativeMenuBar, false);

    QNSApplication *cocoaApplication = [QNSApplication sharedApplication];

    if (qgetenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM").isEmpty()) {
        // Applications launched from plain executables (without an app
        // bundle) are "background" applications that does not take keybaord
        // focus or have a dock icon or task switcher entry. Qt Gui apps generally
        // wants to be foreground applications so change the process type. (But
        // see the function implementation for exceptions.)
        qt_mac_transformProccessToForegroundApplication();

        // Move the application window to front to avoid launching behind the terminal.
        // Ignoring other apps is neccessary (we must ignore the terminal), but makes
        // Qt apps play slightly less nice with other apps when lanching from Finder
        // (See the activateIgnoringOtherApps docs.)
        [cocoaApplication activateIgnoringOtherApps : YES];
    }

    // ### For AA_MacPluginApplication we don't want to load the menu nib.
    // Qt 4 also does not set the application delegate, so that behavior
    // is matched here.
    if (!QCoreApplication::testAttribute(Qt::AA_MacPluginApplication)) {

        // Set app delegate, link to the current delegate (if any)
        QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) *newDelegate = [QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate];
        [newDelegate setReflectionDelegate:[cocoaApplication delegate]];
        [cocoaApplication setDelegate:newDelegate];

        // Load the application menu. This menu contains Preferences, Hide, Quit.
        QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *qtMenuLoader = [[QT_MANGLE_NAMESPACE(QCocoaMenuLoader) alloc] init];
        qt_mac_loadMenuNib(qtMenuLoader);
        [cocoaApplication setMenu:[qtMenuLoader menu]];
        [newDelegate setMenuLoader:qtMenuLoader];
    }

    NSArray *screens = [NSScreen screens];
    for (uint i = 0; i < [screens count]; i++) {
        QCocoaScreen *screen = new QCocoaScreen(i);
        screenAdded(screen);
    }

    mAccessibility = new QPlatformAccessibility;
}

QCocoaIntegration::~QCocoaIntegration()
{
    delete mAccessibility;
    delete mPool;
}

bool QCocoaIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL : return true;
    case ThreadedOpenGL : return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}



QPlatformWindow *QCocoaIntegration::createPlatformWindow(QWindow *window) const
{
    return new QCocoaWindow(window);
}

QPlatformOpenGLContext *QCocoaIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QCocoaGLContext(context->format(), context->shareHandle());
}

QPlatformBackingStore *QCocoaIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QCocoaBackingStore(window);
}

QAbstractEventDispatcher *QCocoaIntegration::guiThreadEventDispatcher() const
{
    return mEventDispatcher;
}

QPlatformFontDatabase *QCocoaIntegration::fontDatabase() const
{
    return mFontDb;
}

QPlatformMenu *QCocoaIntegration::createPlatformMenu(QMenu *menu) const
{
    return new QCocoaMenu(menu);
}

QPlatformMenuBar *QCocoaIntegration::createPlatformMenuBar(QMenuBar *menuBar) const
{
    return new QCocoaMenuBar(menuBar);
}

QPlatformNativeInterface *QCocoaIntegration::nativeInterface() const
{
    return new QCocoaNativeInterface();
}

QPlatformAccessibility *QCocoaIntegration::accessibility() const
{
    return mAccessibility;
}

QT_END_NAMESPACE
