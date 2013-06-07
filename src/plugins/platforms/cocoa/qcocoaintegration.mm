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

#include "qcocoaintegration.h"

#include "qcocoawindow.h"
#include "qcocoabackingstore.h"
#include "qcocoanativeinterface.h"
#include "qcocoamenuloader.h"
#include "qcocoaeventdispatcher.h"
#include "qcocoahelpers.h"
#include "qcocoaapplication.h"
#include "qcocoaapplicationdelegate.h"
#include "qcocoafiledialoghelper.h"
#include "qcocoatheme.h"
#include "qcocoainputcontext.h"
#include "qmacmime.h"
#include "qcocoaaccessibility.h"

#include <qpa/qplatformaccessibility.h>
#include <QtCore/qcoreapplication.h>

#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>
#include <IOKit/graphics/IOGraphicsLib.h>

static void initResources()
{
    Q_INIT_RESOURCE_EXTERN(qcocoaresources)
    Q_INIT_RESOURCE(qcocoaresources);
}

QT_BEGIN_NAMESPACE

QCocoaScreen::QCocoaScreen(int screenIndex) :
    QPlatformScreen(), m_screenIndex(screenIndex), m_refreshRate(60.0)
{
    updateGeometry();
    m_cursor = new QCocoaCursor;
}

QCocoaScreen::~QCocoaScreen()
{
    delete m_cursor;
}

NSScreen *QCocoaScreen::osScreen() const
{
    return [[NSScreen screens] objectAtIndex:m_screenIndex];
}

void QCocoaScreen::updateGeometry()
{
    NSScreen *nsScreen = osScreen();
    NSRect frameRect = [nsScreen frame];

    if (m_screenIndex == 0) {
        m_geometry = QRect(frameRect.origin.x, frameRect.origin.y, frameRect.size.width, frameRect.size.height);
        // This is the primary screen, the one that contains the menubar. Its origin should be
        // (0, 0), and it's the only one whose available geometry differs from its full geometry.
        NSRect visibleRect = [nsScreen visibleFrame];
        m_availableGeometry = QRect(visibleRect.origin.x,
                                    frameRect.size.height - (visibleRect.origin.y + visibleRect.size.height), // invert y
                                    visibleRect.size.width, visibleRect.size.height);
    } else {
        // NSScreen origin is at the bottom-left corner, QScreen is at the top-left corner.
        // When we get the NSScreen frame rect, we need to re-align its origin y coordinate
        // w.r.t. the primary screen, whose origin is (0, 0).
        NSRect r = [[[NSScreen screens] objectAtIndex:0] frame];
        QRect referenceScreenGeometry = QRect(r.origin.x, r.origin.y, r.size.width, r.size.height);
        m_geometry = QRect(frameRect.origin.x,
                           referenceScreenGeometry.height() - (frameRect.origin.y + frameRect.size.height),
                           frameRect.size.width, frameRect.size.height);

        // Not primary screen. See above.
        m_availableGeometry = m_geometry;
    }

    m_format = QImage::Format_RGB32;
    m_depth = NSBitsPerPixelFromDepth([nsScreen depth]);

    NSDictionary *devDesc = [nsScreen deviceDescription];
    CGDirectDisplayID dpy = [[devDesc objectForKey:@"NSScreenNumber"] unsignedIntValue];
    CGSize size = CGDisplayScreenSize(dpy);
    m_physicalSize = QSizeF(size.width, size.height);
    m_logicalDpi.first = 72;
    m_logicalDpi.second = 72;
    CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(dpy);
    float refresh = CGDisplayModeGetRefreshRate(displayMode);
    CGDisplayModeRelease(displayMode);
    if (refresh > 0)
        m_refreshRate = refresh;

    // Get m_name (brand/model of the monitor)
    NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(CGDisplayIOServicePort(dpy), kIODisplayOnlyPreferredName);
    NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];
    if ([localizedNames count] > 0)
        m_name = QString::fromUtf8([[localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]] UTF8String]);
    [deviceInfo release];

    QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry());
    QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(), m_logicalDpi.first, m_logicalDpi.second);
    QWindowSystemInterface::handleScreenRefreshRateChange(screen(), m_refreshRate);
    QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), availableGeometry());
}

qreal QCocoaScreen::devicePixelRatio() const
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        return qreal([osScreen() backingScaleFactor]);
    } else
#endif
    {
        return 1.0;
    }
}

extern CGContextRef qt_mac_cg_context(const QPaintDevice *pdev);

QPixmap QCocoaScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    // TODO window should be handled
    Q_UNUSED(window)

    const int maxDisplays = 128; // 128 displays should be enough for everyone.
    CGDirectDisplayID displays[maxDisplays];
    CGDisplayCount displayCount;
    CGRect cgRect;

    if (width < 0 || height < 0) {
        // get all displays
        cgRect = CGRectInfinite;
    } else {
        cgRect = CGRectMake(x, y, width, height);
    }
    const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);

    if (err && displayCount == 0)
        return QPixmap();

    // calculate pixmap size
    QSize windowSize(width, height);
    if (width < 0 || height < 0) {
        QRect windowRect;
        for (uint i = 0; i < displayCount; ++i) {
            const CGRect cgRect = CGDisplayBounds(displays[i]);
            QRect qRect(cgRect.origin.x, cgRect.origin.y, cgRect.size.width, cgRect.size.height);
            windowRect = windowRect.united(qRect);
        }
        if (width < 0)
            windowSize.setWidth(windowRect.width());
        if (height < 0)
            windowSize.setHeight(windowRect.height());
    }

    QPixmap windowPixmap(windowSize * devicePixelRatio());
    windowPixmap.fill(Qt::transparent);

    for (uint i = 0; i < displayCount; ++i) {
        const CGRect bounds = CGDisplayBounds(displays[i]);
        int w = (width < 0 ? bounds.size.width : width) * devicePixelRatio();
        int h = (height < 0 ? bounds.size.height : height) * devicePixelRatio();
        QRect displayRect = QRect(x, y, w, h);
        QCFType<CGImageRef> image = CGDisplayCreateImageForRect(displays[i],
            CGRectMake(displayRect.x(), displayRect.y(), displayRect.width(), displayRect.height()));
        QPixmap pix(w, h);
        pix.fill(Qt::transparent);
        CGRect rect = CGRectMake(0, 0, w, h);
        CGContextRef ctx = qt_mac_cg_context(&pix);
        qt_mac_drawCGImage(ctx, &rect, image);
        CGContextRelease(ctx);

        QPainter painter(&windowPixmap);
        painter.drawPixmap(bounds.origin.x, bounds.origin.y, pix);
    }
    return windowPixmap;
}

QCocoaIntegration::QCocoaIntegration()
    : mFontDb(new QCoreTextFontDatabase())
    , mEventDispatcher(new QCocoaEventDispatcher())
    , mInputContext(new QCocoaInputContext)
#ifndef QT_NO_ACCESSIBILITY
    , mAccessibility(new QCocoaAccessibility)
#endif
    , mCocoaClipboard(new QCocoaClipboard)
    , mCocoaDrag(new QCocoaDrag)
    , mNativeInterface(new QCocoaNativeInterface)
    , mServices(new QCocoaServices)
    , mKeyboardMapper(new QCocoaKeyMapper)
{
    initResources();
    QCocoaAutoReleasePool pool;

    qApp->setAttribute(Qt::AA_DontUseNativeMenuBar, false);

    NSApplication *cocoaApplication = [QT_MANGLE_NAMESPACE(QNSApplication) sharedApplication];
    qt_redirectNSApplicationSendEvent();

    if (qEnvironmentVariableIsEmpty("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM")) {
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

    updateScreens();

    QMacPasteboardMime::initializeMimeTypes();
}

QCocoaIntegration::~QCocoaIntegration()
{
    qt_resetNSApplicationSendEvent();

    QCocoaAutoReleasePool pool;
    if (!QCoreApplication::testAttribute(Qt::AA_MacPluginApplication)) {
        // remove the apple event handlers installed by QCocoaApplicationDelegate
        QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) *delegate = [QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate];
        [delegate removeAppleEventHandlers];
        // reset the application delegate
        [[NSApplication sharedApplication] setDelegate: 0];
    }

    // Delete the clipboard integration and destroy mime type converters.
    // Deleting the clipboard integration flushes promised pastes using
    // the mime converters - the ordering here is important.
    delete mCocoaClipboard;
    QMacPasteboardMime::destroyMimeTypes();

    // Delete screens in reverse order to avoid crash in case of multiple screens
    while (!mScreens.isEmpty()) {
        delete mScreens.takeLast();
    }
}

/*!
    \brief Synchronizes the screen list, adds new screens, removes deleted ones
*/
void QCocoaIntegration::updateScreens()
{
    NSArray *screens = [NSScreen screens];
    QSet<QCocoaScreen*> remainingScreens = QSet<QCocoaScreen*>::fromList(mScreens);
    QList<QPlatformScreen *> siblings;
    for (uint i = 0; i < [screens count]; i++) {
        NSScreen* scr = [[NSScreen screens] objectAtIndex:i];
        CGDirectDisplayID dpy = [[[scr deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];
        // If this screen is a mirror and is not the primary one of the mirror set, ignore it.
        if (CGDisplayIsInMirrorSet(dpy)) {
            CGDirectDisplayID primary = CGDisplayMirrorsDisplay(dpy);
            if (primary != kCGNullDirectDisplay && primary != dpy)
                continue;
        }
        QCocoaScreen* screen = NULL;
        foreach (QCocoaScreen* existingScr, mScreens)
            // NSScreen documentation says do not cache the array returned from [NSScreen screens].
            // However in practice, we can identify a screen by its pointer: if resolution changes,
            // the NSScreen object will be the same instance, just with different values.
            if (existingScr->osScreen() == scr) {
                screen = existingScr;
                break;
            }
        if (screen) {
            remainingScreens.remove(screen);
            screen->updateGeometry();
        } else {
            screen = new QCocoaScreen(i);
            mScreens.append(screen);
            screenAdded(screen);
        }
        siblings << screen;
    }
    // Now the leftovers in remainingScreens are no longer current, so we can delete them.
    foreach (QCocoaScreen* screen, remainingScreens) {
        mScreens.removeOne(screen);
        delete screen;
    }
    // All screens in mScreens are siblings, because we ignored the mirrors.
    foreach (QCocoaScreen* screen, mScreens)
        screen->setVirtualSiblings(siblings);
}

bool QCocoaIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case OpenGL:
    case ThreadedOpenGL:
    case BufferQueueingOpenGL:
    case WindowMasks:
    case MultipleWindows:
    case ForeignWindows:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
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
    return mFontDb.data();
}

QPlatformNativeInterface *QCocoaIntegration::nativeInterface() const
{
    return mNativeInterface.data();
}

QPlatformInputContext *QCocoaIntegration::inputContext() const
{
    return mInputContext.data();
}

QPlatformAccessibility *QCocoaIntegration::accessibility() const
{
#ifndef QT_NO_ACCESSIBILITY
    return mAccessibility.data();
#else
    return 0;
#endif
}

QPlatformClipboard *QCocoaIntegration::clipboard() const
{
    return mCocoaClipboard;
}

QPlatformDrag *QCocoaIntegration::drag() const
{
    return mCocoaDrag.data();
}

QStringList QCocoaIntegration::themeNames() const
{
    return QStringList(QLatin1String(QCocoaTheme::name));
}

QPlatformTheme *QCocoaIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QCocoaTheme::name))
        return new QCocoaTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformServices *QCocoaIntegration::services() const
{
    return mServices.data();
}

QVariant QCocoaIntegration::styleHint(StyleHint hint) const
{
    if (hint == QPlatformIntegration::FontSmoothingGamma)
        return 2.0;
    if (hint == QPlatformIntegration::SynthesizeMouseFromTouchEvents)
        return false;

    return QPlatformIntegration::styleHint(hint);
}

QList<int> QCocoaIntegration::possibleKeys(const QKeyEvent *event) const
{
    return mKeyboardMapper->possibleKeys(event);
}

QT_END_NAMESPACE
