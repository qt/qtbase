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

#include <qpa/qplatformtheme.h>

#include "qcocoahelpers.h"
#include "qnsview.h"

#include <QtCore>
#include <QtGui>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>
#include <QtGui/private/qcoregraphics_p.h>

#ifndef QT_NO_WIDGETS
#include <QtWidgets/QWidget>
#endif

#include <algorithm>

#include <Carbon/Carbon.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaCocoaWindow, "qt.qpa.cocoa.window");

//
// Conversion Functions
//

QStringList qt_mac_NSArrayToQStringList(void *nsarray)
{
    QStringList result;
    NSArray *array = static_cast<NSArray *>(nsarray);
    for (NSUInteger i=0; i<[array count]; ++i)
        result << QString::fromNSString([array objectAtIndex:i]);
    return result;
}

void *qt_mac_QStringListToNSMutableArrayVoid(const QStringList &list)
{
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:list.size()];
    for (int i=0; i<list.size(); ++i){
        [result addObject:list[i].toNSString()];
    }
    return result;
}

struct dndenum_mapper
{
    NSDragOperation mac_code;
    Qt::DropAction qt_code;
    bool Qt2Mac;
};

static dndenum_mapper dnd_enums[] = {
    { NSDragOperationLink,  Qt::LinkAction, true },
    { NSDragOperationMove,  Qt::MoveAction, true },
    { NSDragOperationCopy,  Qt::CopyAction, true },
    { NSDragOperationGeneric,  Qt::CopyAction, false },
    { NSDragOperationEvery, Qt::ActionMask, false },
    { NSDragOperationNone, Qt::IgnoreAction, false }
};

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action)
{
    for (int i=0; dnd_enums[i].qt_code; i++) {
        if (dnd_enums[i].Qt2Mac && (action & dnd_enums[i].qt_code)) {
            return dnd_enums[i].mac_code;
        }
    }
    return NSDragOperationNone;
}

NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions)
{
    NSDragOperation nsActions = NSDragOperationNone;
    for (int i=0; dnd_enums[i].qt_code; i++) {
        if (dnd_enums[i].Qt2Mac && (actions & dnd_enums[i].qt_code))
            nsActions |= dnd_enums[i].mac_code;
    }
    return nsActions;
}

Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions)
{
    Qt::DropAction action = Qt::IgnoreAction;
    for (int i=0; dnd_enums[i].mac_code; i++) {
        if (nsActions & dnd_enums[i].mac_code)
            return dnd_enums[i].qt_code;
    }
    return action;
}

Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions)
{
    Qt::DropActions actions = Qt::IgnoreAction;

    for (int i=0; dnd_enums[i].mac_code; i++) {
        if (dnd_enums[i].mac_code == NSDragOperationEvery)
            continue;

        if (nsActions & dnd_enums[i].mac_code)
            actions |= dnd_enums[i].qt_code;
    }
    return actions;
}

/*!
    Returns the view cast to a QNSview if possible.

    If the view is not a QNSView, nil is returned, which is safe to
    send messages to, effectivly making [qnsview_cast(view) message]
    a no-op.

    For extra verbosity and clearer code, please consider checking
    that window()->type() != Qt::ForeignWindow before using this cast.

    Do not use this method soley to check for foreign windows, as
    that will make the code harder to read for people not working
    primarily on macOS, who do not know the difference between the
    NSView and QNSView cases.
*/
QNSView *qnsview_cast(NSView *view)
{
    if (![view isKindOfClass:[QNSView class]]) {
        qCWarning(lcQpaCocoaWindow) << "NSView is not QNSView, consider checking for Qt::ForeignWindow";
        return nil;
    }

    return static_cast<QNSView *>(view);
}

//
// Misc
//

// Sets the activation policy for this process to NSApplicationActivationPolicyRegular,
// unless either LSUIElement or LSBackgroundOnly is set in the Info.plist.
void qt_mac_transformProccessToForegroundApplication()
{
    bool forceTransform = true;
    CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                           CFSTR("LSUIElement"));
    if (value) {
        CFTypeID valueType = CFGetTypeID(value);
        // Officially it's supposed to be a string, a boolean makes sense, so we'll check.
        // A number less so, but OK.
        if (valueType == CFStringGetTypeID())
            forceTransform = !(QString::fromCFString(static_cast<CFStringRef>(value)).toInt());
        else if (valueType == CFBooleanGetTypeID())
            forceTransform = !CFBooleanGetValue(static_cast<CFBooleanRef>(value));
        else if (valueType == CFNumberGetTypeID()) {
            int valueAsInt;
            CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
            forceTransform = !valueAsInt;
        }
    }

    if (forceTransform) {
        value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                     CFSTR("LSBackgroundOnly"));
        if (value) {
            CFTypeID valueType = CFGetTypeID(value);
            if (valueType == CFBooleanGetTypeID())
                forceTransform = !CFBooleanGetValue(static_cast<CFBooleanRef>(value));
            else if (valueType == CFStringGetTypeID())
                forceTransform = !(QString::fromCFString(static_cast<CFStringRef>(value)).toInt());
            else if (valueType == CFNumberGetTypeID()) {
                int valueAsInt;
                CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
                forceTransform = !valueAsInt;
            }
        }
    }

    if (forceTransform) {
        [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
}

QString qt_mac_applicationName()
{
    QString appName;
    CFTypeRef string = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleName"));
    if (string)
        appName = QString::fromCFString(static_cast<CFStringRef>(string));

    if (appName.isEmpty()) {
        QString arg0 = QGuiApplicationPrivate::instance()->appName();
        if (arg0.contains("/")) {
            QStringList parts = arg0.split(QLatin1Char('/'));
            appName = parts.at(parts.count() - 1);
        } else {
            appName = arg0;
        }
    }
    return appName;
}

int qt_mac_primaryScreenHeight()
{
    QMacAutoReleasePool pool;
    NSArray *screens = [NSScreen screens];
    if ([screens count] > 0) {
        // The first screen in the screens array is documented to
        // have the (0,0) origin and is designated the primary screen.
        NSRect screenFrame = [[screens objectAtIndex: 0] frame];
        return screenFrame.size.height;
    }
    return 0;
}

int qt_mac_flipYCoordinate(int y)
{
    return qt_mac_primaryScreenHeight() - y;
}

qreal qt_mac_flipYCoordinate(qreal y)
{
    return qt_mac_primaryScreenHeight() - y;
}

QPointF qt_mac_flipPoint(const NSPoint &p)
{
    return QPointF(p.x, qt_mac_flipYCoordinate(p.y));
}

NSPoint qt_mac_flipPoint(const QPoint &p)
{
    return NSMakePoint(p.x(), qt_mac_flipYCoordinate(p.y()));
}

NSPoint qt_mac_flipPoint(const QPointF &p)
{
    return NSMakePoint(p.x(), qt_mac_flipYCoordinate(p.y()));
}

NSRect qt_mac_flipRect(const QRect &rect)
{
    int flippedY = qt_mac_flipYCoordinate(rect.y() + rect.height());
    return NSMakeRect(rect.x(), flippedY, rect.width(), rect.height());
}

Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum)
{
    if (buttonNum == 0)
        return Qt::LeftButton;
    if (buttonNum == 1)
        return Qt::RightButton;
    if (buttonNum == 2)
        return Qt::MiddleButton;
    if (buttonNum >= 3 && buttonNum <= 31) { // handle XButton1 and higher via logical shift
        return Qt::MouseButton(uint(Qt::MiddleButton) << (buttonNum - 3));
    }
    // else error: buttonNum too high, or negative
    return Qt::NoButton;
}

QString qt_mac_removeAmpersandEscapes(QString s)
{
    return QPlatformTheme::removeMnemonics(s).trimmed();
}

QT_END_NAMESPACE
