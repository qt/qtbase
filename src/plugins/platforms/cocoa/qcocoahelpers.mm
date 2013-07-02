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

#include "qcocoahelpers.h"

#include "qcocoaautoreleasepool.h"

#include <QtCore>
#include <QtGui>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>

#ifndef QT_NO_WIDGETS
#include <QtWidgets/QWidget>
#endif

QT_BEGIN_NAMESPACE

//
// Conversion Functions
//

QStringList qt_mac_NSArrayToQStringList(void *nsarray)
{
    QStringList result;
    NSArray *array = static_cast<NSArray *>(nsarray);
    for (NSUInteger i=0; i<[array count]; ++i)
        result << QCFString::toQString([array objectAtIndex:i]);
    return result;
}

void *qt_mac_QStringListToNSMutableArrayVoid(const QStringList &list)
{
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:list.size()];
    for (int i=0; i<list.size(); ++i){
        [result addObject:reinterpret_cast<const NSString *>(QCFString::toCFStringRef(list[i]))];
    }
    return result;
}

static void drawImageReleaseData (void *info, const void *, size_t)
{
    delete static_cast<QImage *>(info);
}

CGImageRef qt_mac_image_to_cgimage(const QImage &img)
{
    if (img.isNull())
        return 0;

    QImage *image;
    if (img.depth() != 32)
        image = new QImage(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    else
        image = new QImage(img);

    uint cgflags = kCGImageAlphaNone;
    switch (image->format()) {
    case QImage::Format_ARGB32_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedFirst;
        break;
    case QImage::Format_ARGB32:
        cgflags = kCGImageAlphaFirst;
        break;
    case QImage::Format_RGB32:
        cgflags = kCGImageAlphaNoneSkipFirst;
    default:
        break;
    }
    cgflags |= kCGBitmapByteOrder32Host;
    QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData(image,
                                                          static_cast<const QImage *>(image)->bits(),
                                                          image->byteCount(),
                                                          drawImageReleaseData);

    return CGImageCreate(image->width(), image->height(), 8, 32,
                                        image->bytesPerLine(),
                                        qt_mac_genericColorSpace(),
                                        cgflags, dataProvider, 0, false, kCGRenderingIntentDefault);

}

NSImage *qt_mac_cgimage_to_nsimage(CGImageRef image)
{
    NSImage *newImage = [[NSImage alloc] initWithCGImage:image size:NSZeroSize];
    return newImage;
}

NSImage *qt_mac_create_nsimage(const QPixmap &pm)
{
    if (pm.isNull())
        return 0;
    QImage image = pm.toImage();
    CGImageRef cgImage = qt_mac_image_to_cgimage(image);
    NSImage *nsImage = qt_mac_cgimage_to_nsimage(cgImage);
    CGImageRelease(cgImage);
    return nsImage;
}

NSImage *qt_mac_create_nsimage(const QIcon &icon)
{
    if (icon.isNull())
        return nil;

    NSImage *nsImage = [[NSImage alloc] init];
    foreach (QSize size, icon.availableSizes()) {
        QPixmap pm = icon.pixmap(size);
        QImage image = pm.toImage();
        CGImageRef cgImage = qt_mac_image_to_cgimage(image);
        NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        [nsImage addRepresentation:imageRep];
        [imageRep release];
        CGImageRelease(cgImage);
    }
    return nsImage;
}

HIMutableShapeRef qt_mac_QRegionToHIMutableShape(const QRegion &region)
{
    HIMutableShapeRef shape = HIShapeCreateMutable();
    QVector<QRect> rects = region.rects();
    if (!rects.isEmpty()) {
        int n = rects.count();
        const QRect *qt_r = rects.constData();
        while (n--) {
            CGRect cgRect = CGRectMake(qt_r->x(), qt_r->y(), qt_r->width(), qt_r->height());
            HIShapeUnionWithRect(shape, &cgRect);
            ++qt_r;
        }
    }
    return shape;
}

NSSize qt_mac_toNSSize(const QSize &qtSize)
{
    return NSMakeSize(qtSize.width(), qtSize.height());
}

NSRect qt_mac_toNSRect(const QRect &rect)
{
    return NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
}

QRect qt_mac_toQRect(const NSRect &rect)
{
    return QRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

QColor qt_mac_toQColor(const NSColor *color)
{
    QColor qtColor;
    NSString *colorSpace = [color colorSpaceName];
    if (colorSpace == NSDeviceCMYKColorSpace) {
        CGFloat cyan, magenta, yellow, black, alpha;
        [color getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        qtColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } else {
        NSColor *tmpColor;
        tmpColor = [color colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        CGFloat red, green, blue, alpha;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        qtColor.setRgbF(red, green, blue, alpha);
    }
    return qtColor;
}


// Use this method to keep all the information in the TextSegment. As long as it is ordered
// we are in OK shape, and we can influence that ourselves.
struct KeyPair
{
    QChar cocoaKey;
    Qt::Key qtKey;
};

bool operator==(const KeyPair &entry, QChar qchar)
{
    return entry.cocoaKey == qchar;
}

bool operator<(const KeyPair &entry, QChar qchar)
{
    return entry.cocoaKey < qchar;
}

bool operator<(QChar qchar, const KeyPair &entry)
{
    return qchar < entry.cocoaKey;
}

bool operator<(const Qt::Key &key, const KeyPair &entry)
{
    return key < entry.qtKey;
}

bool operator<(const KeyPair &entry, const Qt::Key &key)
{
    return entry.qtKey < key;
}

static bool qtKey2CocoaKeySortLessThan(const KeyPair &entry1, const KeyPair &entry2)
{
    return entry1.qtKey < entry2.qtKey;
}

static const int NumEntries = 59;
static const KeyPair entries[NumEntries] = {
    { NSEnterCharacter, Qt::Key_Enter },
    { NSBackspaceCharacter, Qt::Key_Backspace },
    { NSTabCharacter, Qt::Key_Tab },
    { NSNewlineCharacter, Qt::Key_Return },
    { NSCarriageReturnCharacter, Qt::Key_Return },
    { NSBackTabCharacter, Qt::Key_Backtab },
    { kEscapeCharCode, Qt::Key_Escape },
    // Cocoa sends us delete when pressing backspace!
    // (NB when we reverse this list in qtKey2CocoaKey, there
    // will be two indices of Qt::Key_Backspace. But is seems to work
    // ok for menu shortcuts (which uses that function):
    { NSDeleteCharacter, Qt::Key_Backspace },
    { NSUpArrowFunctionKey, Qt::Key_Up },
    { NSDownArrowFunctionKey, Qt::Key_Down },
    { NSLeftArrowFunctionKey, Qt::Key_Left },
    { NSRightArrowFunctionKey, Qt::Key_Right },
    { NSF1FunctionKey, Qt::Key_F1 },
    { NSF2FunctionKey, Qt::Key_F2 },
    { NSF3FunctionKey, Qt::Key_F3 },
    { NSF4FunctionKey, Qt::Key_F4 },
    { NSF5FunctionKey, Qt::Key_F5 },
    { NSF6FunctionKey, Qt::Key_F6 },
    { NSF7FunctionKey, Qt::Key_F7 },
    { NSF8FunctionKey, Qt::Key_F8 },
    { NSF9FunctionKey, Qt::Key_F9 },
    { NSF10FunctionKey, Qt::Key_F10 },
    { NSF11FunctionKey, Qt::Key_F11 },
    { NSF12FunctionKey, Qt::Key_F12 },
    { NSF13FunctionKey, Qt::Key_F13 },
    { NSF14FunctionKey, Qt::Key_F14 },
    { NSF15FunctionKey, Qt::Key_F15 },
    { NSF16FunctionKey, Qt::Key_F16 },
    { NSF17FunctionKey, Qt::Key_F17 },
    { NSF18FunctionKey, Qt::Key_F18 },
    { NSF19FunctionKey, Qt::Key_F19 },
    { NSF20FunctionKey, Qt::Key_F20 },
    { NSF21FunctionKey, Qt::Key_F21 },
    { NSF22FunctionKey, Qt::Key_F22 },
    { NSF23FunctionKey, Qt::Key_F23 },
    { NSF24FunctionKey, Qt::Key_F24 },
    { NSF25FunctionKey, Qt::Key_F25 },
    { NSF26FunctionKey, Qt::Key_F26 },
    { NSF27FunctionKey, Qt::Key_F27 },
    { NSF28FunctionKey, Qt::Key_F28 },
    { NSF29FunctionKey, Qt::Key_F29 },
    { NSF30FunctionKey, Qt::Key_F30 },
    { NSF31FunctionKey, Qt::Key_F31 },
    { NSF32FunctionKey, Qt::Key_F32 },
    { NSF33FunctionKey, Qt::Key_F33 },
    { NSF34FunctionKey, Qt::Key_F34 },
    { NSF35FunctionKey, Qt::Key_F35 },
    { NSInsertFunctionKey, Qt::Key_Insert },
    { NSDeleteFunctionKey, Qt::Key_Delete },
    { NSHomeFunctionKey, Qt::Key_Home },
    { NSEndFunctionKey, Qt::Key_End },
    { NSPageUpFunctionKey, Qt::Key_PageUp },
    { NSPageDownFunctionKey, Qt::Key_PageDown },
    { NSPrintScreenFunctionKey, Qt::Key_Print },
    { NSScrollLockFunctionKey, Qt::Key_ScrollLock },
    { NSPauseFunctionKey, Qt::Key_Pause },
    { NSSysReqFunctionKey, Qt::Key_SysReq },
    { NSMenuFunctionKey, Qt::Key_Menu },
    { NSHelpFunctionKey, Qt::Key_Help },
};
static const KeyPair * const end = entries + NumEntries;

QChar qt_mac_qtKey2CocoaKey(Qt::Key key)
{
    // The first time this function is called, create a reverse
    // lookup table sorted on Qt Key rather than Cocoa key:
    static QVector<KeyPair> rev_entries(NumEntries);
    static bool mustInit = true;
    if (mustInit){
        mustInit = false;
        for (int i=0; i<NumEntries; ++i)
            rev_entries[i] = entries[i];
        qSort(rev_entries.begin(), rev_entries.end(), qtKey2CocoaKeySortLessThan);
    }
    const QVector<KeyPair>::iterator i
            = qBinaryFind(rev_entries.begin(), rev_entries.end(), key);
    if (i == rev_entries.end())
        return QChar();
    return i->cocoaKey;
}

Qt::Key qt_mac_cocoaKey2QtKey(QChar keyCode)
{
    const KeyPair *i = qBinaryFind(entries, end, keyCode);
    if (i == end)
        return Qt::Key(keyCode.toUpper().unicode());
    return i->qtKey;
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



//
// Misc
//

// Changes the process type for this process to kProcessTransformToForegroundApplication,
// unless either LSUIElement or LSBackgroundOnly is set in the Info.plist.
void qt_mac_transformProccessToForegroundApplication()
{
    ProcessSerialNumber psn;
    if (GetCurrentProcess(&psn) == noErr) {
        bool forceTransform = true;
        CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                               CFSTR("LSUIElement"));
        if (value) {
            CFTypeID valueType = CFGetTypeID(value);
            // Officially it's supposed to be a string, a boolean makes sense, so we'll check.
            // A number less so, but OK.
            if (valueType == CFStringGetTypeID())
                forceTransform = !(QCFString::toQString(static_cast<CFStringRef>(value)).toInt());
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
                    forceTransform = !(QCFString::toQString(static_cast<CFStringRef>(value)).toInt());
                else if (valueType == CFNumberGetTypeID()) {
                    int valueAsInt;
                    CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
                    forceTransform = !valueAsInt;
                }
            }
        }

        if (forceTransform) {
            TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        }
    }
}

QString qt_mac_removeMnemonics(const QString &original)
{
    QString returnText(original.size(), 0);
    int finalDest = 0;
    int currPos = 0;
    int l = original.length();
    while (l) {
        if (original.at(currPos) == QLatin1Char('&')
            && (l == 1 || original.at(currPos + 1) != QLatin1Char('&'))) {
            ++currPos;
            --l;
            if (l == 0)
                break;
        }
        returnText[finalDest] = original.at(currPos);
        ++currPos;
        ++finalDest;
        --l;
    }
    returnText.truncate(finalDest);
    return returnText;
}

static CGColorSpaceRef m_genericColorSpace = 0;
static QHash<CGDirectDisplayID, CGColorSpaceRef> m_displayColorSpaceHash;
static bool m_postRoutineRegistered = false;

CGColorSpaceRef qt_mac_genericColorSpace()
{
#if 0
    if (!m_genericColorSpace) {
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
            m_genericColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        } else
        {
            m_genericColorSpace = CGColorSpaceCreateDeviceRGB();
        }
        if (!m_postRoutineRegistered) {
            m_postRoutineRegistered = true;
            qAddPostRoutine(QCoreGraphicsPaintEngine::cleanUpMacColorSpaces);
        }
    }
    return m_genericColorSpace;
#else
    // Just return the main display colorspace for the moment.
    return qt_mac_displayColorSpace(0);
#endif
}

/*
    Ideally, we should pass the widget in here, and use CGGetDisplaysWithRect() etc.
    to support multiple displays correctly.
*/
CGColorSpaceRef qt_mac_displayColorSpace(const QWidget *widget)
{
    CGColorSpaceRef colorSpace;

    CGDirectDisplayID displayID;
    CMProfileRef displayProfile = 0;
    if (widget == 0) {
        displayID = CGMainDisplayID();
    } else {
        displayID = CGMainDisplayID();
        /*
        ### get correct display
        const QRect &qrect = widget->window()->geometry();
        CGRect rect = CGRectMake(qrect.x(), qrect.y(), qrect.width(), qrect.height());
        CGDisplayCount throwAway;
        CGDisplayErr dErr = CGGetDisplaysWithRect(rect, 1, &displayID, &throwAway);
        if (dErr != kCGErrorSuccess)
            return macDisplayColorSpace(0); // fall back on main display
        */
    }
    if ((colorSpace = m_displayColorSpaceHash.value(displayID)))
        return colorSpace;

    CMError err = CMGetProfileByAVID((CMDisplayIDType)displayID, &displayProfile);
    if (err == noErr) {
        colorSpace = CGColorSpaceCreateWithPlatformColorSpace(displayProfile);
    } else if (widget) {
        return qt_mac_displayColorSpace(0); // fall back on main display
    }

    if (colorSpace == 0)
        colorSpace = CGColorSpaceCreateDeviceRGB();

    m_displayColorSpaceHash.insert(displayID, colorSpace);
    CMCloseProfile(displayProfile);
    if (!m_postRoutineRegistered) {
        m_postRoutineRegistered = true;
        void qt_mac_cleanUpMacColorSpaces();
        qAddPostRoutine(qt_mac_cleanUpMacColorSpaces);
    }
    return colorSpace;
}

void qt_mac_cleanUpMacColorSpaces()
{
    if (m_genericColorSpace) {
        CFRelease(m_genericColorSpace);
        m_genericColorSpace = 0;
    }
    QHash<CGDirectDisplayID, CGColorSpaceRef>::const_iterator it = m_displayColorSpaceHash.constBegin();
    while (it != m_displayColorSpaceHash.constEnd()) {
        if (it.value())
            CFRelease(it.value());
        ++it;
    }
    m_displayColorSpaceHash.clear();
}

CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *paintDevice)
{
#ifdef QT_NO_WIDGETS
    return qt_mac_displayColorSpace(0);
#else
    bool isWidget = (paintDevice->devType() == QInternal::Widget);
    return qt_mac_displayColorSpace(isWidget ? static_cast<const QWidget *>(paintDevice): 0);
#endif

}

QString qt_mac_applicationName()
{
    QString appName;
    CFTypeRef string = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleName"));
    if (string)
        appName = QCFString::toQString(static_cast<CFStringRef>(string));

    if (appName.isEmpty()) {
        QString arg0 = QGuiApplicationPrivate::instance()->appName();
        if (arg0.contains("/")) {
            QStringList parts = arg0.split("/");
            appName = parts.at(parts.count() - 1);
        } else {
            appName = arg0;
        }
    }
    return appName;
}

/*
    Mac window coordinates are in the first quadrant: 0, 0 is at the lower-left
    corner of the primary screen. This function converts the given rect to an
    NSRect for the window geometry, flipping from 4th quadrant to 1st quadrant
    and simultaneously ensuring that as much of the window as possible will be
    onscreen. If the rect is too tall for the screen, the OS will reduce the
    window's height anyway; but by moving the window upwards we can have more
    of it onscreen.  But the application can still control the y coordinate
    in case it really wants the window to be positioned partially offscreen.
*/
NSRect qt_mac_flipRect(const QRect &rect, QWindow *window)
{
    QPlatformScreen *onScreen = QPlatformScreen::platformScreenForWindow(window);
    int flippedY = onScreen->geometry().height() - (rect.y() + rect.height());

    // In case of automatic positioning, try to put as much of the window onscreen as possible.
    if (window->isTopLevel() && qt_window_private(const_cast<QWindow*>(window))->positionAutomatic && flippedY < 0)
        flippedY = onScreen->geometry().height() - onScreen->availableGeometry().height() - onScreen->availableGeometry().y();
#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << Q_FUNC_INFO << rect << "flippedY" << flippedY <<
                "screen" << onScreen->geometry() << "available" << onScreen->availableGeometry();
#endif
    return NSMakeRect(rect.x(), flippedY, rect.width(), rect.height());
}

OSStatus qt_mac_drawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage)
{
    // Verbatim copy if HIViewDrawCGImage (as shown on Carbon-Dev)
    OSStatus err = noErr;

    require_action(inContext != NULL, InvalidContext, err = paramErr);
    require_action(inBounds != NULL, InvalidBounds, err = paramErr);
    require_action(inImage != NULL, InvalidImage, err = paramErr);

    CGContextSaveGState( inContext );
    CGContextTranslateCTM (inContext, 0, inBounds->origin.y + CGRectGetMaxY(*inBounds));
    CGContextScaleCTM(inContext, 1, -1);

    CGContextDrawImage(inContext, *inBounds, inImage);

    CGContextRestoreGState(inContext);
InvalidImage:
InvalidBounds:
InvalidContext:
        return err;
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

bool qt_mac_execute_apple_script(const char *script, long script_len, AEDesc *ret) {
    OSStatus err;
    AEDesc scriptTextDesc;
    ComponentInstance theComponent = 0;
    OSAID scriptID = kOSANullScript, resultID = kOSANullScript;

    // set up locals to a known state
    AECreateDesc(typeNull, 0, 0, &scriptTextDesc);
    scriptID = kOSANullScript;
    resultID = kOSANullScript;

    // open the scripting component
    theComponent = OpenDefaultComponent(kOSAComponentType, typeAppleScript);
    if (!theComponent) {
        err = paramErr;
        goto bail;
    }

    // put the script text into an aedesc
    err = AECreateDesc(typeUTF8Text, script, script_len, &scriptTextDesc);
    if (err != noErr)
        goto bail;

    // compile the script
    err = OSACompile(theComponent, &scriptTextDesc, kOSAModeNull, &scriptID);
    if (err != noErr)
        goto bail;

    // run the script
    err = OSAExecute(theComponent, scriptID, kOSANullScript, kOSAModeNull, &resultID);

    // collect the results - if any
    if (ret) {
        AECreateDesc(typeNull, 0, 0, ret);
        if (err == errOSAScriptError)
            OSAScriptError(theComponent, kOSAErrorMessage, typeChar, ret);
        else if (err == noErr && resultID != kOSANullScript)
            OSADisplay(theComponent, resultID, typeChar, kOSAModeNull, ret);
    }
bail:
    AEDisposeDesc(&scriptTextDesc);
    if (scriptID != kOSANullScript)
        OSADispose(theComponent, scriptID);
    if (resultID != kOSANullScript)
        OSADispose(theComponent, resultID);
    if (theComponent)
        CloseComponent(theComponent);
    return err == noErr;
}

bool qt_mac_execute_apple_script(const char *script, AEDesc *ret)
{
    return qt_mac_execute_apple_script(script, qstrlen(script), ret);
}

bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret)
{
    const QByteArray l = script.toUtf8(); return qt_mac_execute_apple_script(l.constData(), l.size(), ret);
}

QString qt_mac_removeAmpersandEscapes(QString s)
{
    int i = 0;
    while (i < s.size()) {
        ++i;
        if (s.at(i-1) != QLatin1Char('&'))
            continue;
        if (i < s.size() && s.at(i) == QLatin1Char('&'))
            ++i;
        s.remove(i-1,1);
    }
    return s.trimmed();
}

/*! \internal

 Returns the CoreGraphics CGContextRef of the paint device. 0 is
 returned if it can't be obtained. It is the caller's responsibility to
 CGContextRelease the context when finished using it.

 \warning This function is only available on Mac OS X.
 \warning This function is duplicated in qmacstyle_mac.mm
 */
CGContextRef qt_mac_cg_context(QPaintDevice *pdev)
{
    // In Qt 5, QWidget and QPixmap (and QImage) paint devices are all QImages under the hood.
    QImage *image = 0;
    if (pdev->devType() == QInternal::Image) {
        image = static_cast<QImage *>(pdev);
    } else if (pdev->devType() == QInternal::Pixmap) {

        const QPixmap *pm = static_cast<const QPixmap*>(pdev);
        QPlatformPixmap *data = const_cast<QPixmap *>(pm)->data_ptr().data();
        if (data && data->classId() == QPlatformPixmap::RasterClass) {
            image = data->buffer();
        } else {
            qDebug() << "qt_mac_cg_context: Unsupported pixmap class";
        }
    } else if (pdev->devType() == QInternal::Widget) {
        // TODO test: image = static_cast<QImage *>(static_cast<const QWidget *>(pdev)->backingStore()->paintDevice());
        qDebug() << "qt_mac_cg_context: not implemented: Widget class";
    }

    if (!image)
        return 0; // Context type not supported.

    CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(pdev);
    uint flags = kCGImageAlphaPremultipliedFirst;
    flags |= kCGBitmapByteOrder32Host;
    CGContextRef ret = 0;
    ret = CGBitmapContextCreate(image->bits(), image->width(), image->height(),
                                8, image->bytesPerLine(), colorspace, flags);
    CGContextTranslateCTM(ret, 0, image->height());
    CGContextScaleCTM(ret, 1, -1);
    return ret;
}

// qpaintengine_mac.mm
extern void qt_mac_cgimage_data_free(void *, const void *memoryToFree, size_t);

CGImageRef qt_mac_toCGImage(const QImage &qImage, bool isMask, uchar **dataCopy)
{
    int width = qImage.width();
    int height = qImage.height();

    if (width <= 0 || height <= 0) {
        qWarning() << Q_FUNC_INFO <<
            "setting invalid size" << width << "x" << height << "for qnsview image";
        return 0;
    }

    const uchar *imageData = qImage.bits();
    if (dataCopy) {
        *dataCopy = static_cast<uchar *>(malloc(qImage.byteCount()));
        memcpy(*dataCopy, imageData, qImage.byteCount());
    }
    int bitDepth = qImage.depth();
    int colorBufferSize = 8;
    int bytesPrLine = qImage.bytesPerLine();

    CGDataProviderRef cgDataProviderRef = CGDataProviderCreateWithData(
                NULL,
                dataCopy ? *dataCopy : imageData,
                qImage.byteCount(),
                dataCopy ? qt_mac_cgimage_data_free : NULL);

    CGImageRef cgImage = 0;
    if (isMask) {
        cgImage = CGImageMaskCreate(width,
                                    height,
                                    colorBufferSize,
                                    bitDepth,
                                    bytesPrLine,
                                    cgDataProviderRef,
                                    NULL,
                                    false);
    } else {
        // Try get a device color space. Using the device color space means
        // that the CGImage can be drawn to screen without per-pixel color
        // space conversion, at the cost of less color accuracy.
        CGColorSpaceRef cgColourSpaceRef = 0;
        CMProfileRef sysProfile;
        if (CMGetSystemProfile(&sysProfile) == noErr)
        {
            cgColourSpaceRef = CGColorSpaceCreateWithPlatformColorSpace(sysProfile);
            CMCloseProfile(sysProfile);
        }

        // Fall back to Generic RGB if a profile was not found.
        if (!cgColourSpaceRef)
            cgColourSpaceRef = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

        // Create a CGBitmapInfo contiaining the image format.
        // Support the 8-bit per component (A)RGB formats.
        CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Little;
        switch (qImage.format()) {
            case QImage::Format_ARGB32_Premultiplied :
                bitmapInfo |= kCGImageAlphaPremultipliedFirst;
            break;
            case QImage::Format_ARGB32 :
                bitmapInfo |= kCGImageAlphaFirst;
            break;
            case QImage::Format_RGB32 :
                bitmapInfo |= kCGImageAlphaNoneSkipFirst;
            break;
            case QImage::Format_RGB888 :
                bitmapInfo |= kCGImageAlphaNone;
            break;
            default:
                qWarning() << "qt_mac_toCGImage: Unsupported image format" << qImage.format();
            break;
        }

        cgImage = CGImageCreate(width,
                                height,
                                colorBufferSize,
                                bitDepth,
                                bytesPrLine,
                                cgColourSpaceRef,
                                bitmapInfo,
                                cgDataProviderRef,
                                NULL,
                                false,
                                kCGRenderingIntentDefault);
        CGColorSpaceRelease(cgColourSpaceRef);
    }
    CGDataProviderRelease(cgDataProviderRef);
    return cgImage;
}

QImage qt_mac_toQImage(CGImageRef image)
{
    const size_t w = CGImageGetWidth(image),
                 h = CGImageGetHeight(image);
    QImage ret(w, h, QImage::Format_ARGB32_Premultiplied);
    ret.fill(Qt::transparent);
    CGRect rect = CGRectMake(0, 0, w, h);
    CGContextRef ctx = qt_mac_cg_context(&ret);
    qt_mac_drawCGImage(ctx, &rect, image);
    CGContextRelease(ctx);
    return ret;
}


QT_END_NAMESPACE
