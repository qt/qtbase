/****************************************************************************
 **
 ** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qcocoahelpers.h"

#include "qcocoaautoreleasepool.h"

#include <QtCore>
#include <QtGui>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

//
// Conversion Functions
//

QStringList qt_mac_NSArrayToQStringList(void *nsarray)
{
    QStringList result;
    NSArray *array = static_cast<NSArray *>(nsarray);
    for (NSUInteger i=0; i<[array count]; ++i)
        result << qt_mac_NSStringToQString([array objectAtIndex:i]);
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
    QCocoaAutoReleasePool pool;
    NSImage *newImage = 0;
    NSRect imageRect = NSMakeRect(0.0, 0.0, CGImageGetWidth(image), CGImageGetHeight(image));
    newImage = [[NSImage alloc] initWithSize:imageRect.size];
    [newImage lockFocus];
    {
        CGContextRef imageContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
        CGContextDrawImage(imageContext, *(CGRect*)&imageRect, image);
    }
    [newImage unlockFocus];
    return newImage;
}

NSImage *qt_mac_create_nsimage(const QPixmap &pm)
{
    QImage image = pm.toImage();
    return qt_mac_cgimage_to_nsimage(qt_mac_image_to_cgimage(image));
}

NSSize qt_mac_toNSSize(const QSize &qtSize)
{
    return NSMakeSize(qtSize.width(), qtSize.height());
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
    { NSF9FunctionKey, Qt::Key_F8 },
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


CGColorSpaceRef m_genericColorSpace = 0;
QHash<CGDirectDisplayID, CGColorSpaceRef> m_displayColorSpaceHash;
bool m_postRoutineRegistered = false;

CGColorSpaceRef qt_mac_genericColorSpace()
{
#if 0
    if (!m_genericColorSpace) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
            m_genericColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        } else
#endif
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

NSRect qt_mac_flipRect(const QRect &rect, QWindow *window)
{
    QPlatformScreen *onScreen = QPlatformScreen::platformScreenForWindow(window);
    int flippedY = onScreen->geometry().height() - rect.y() - rect.height();
    return NSMakeRect(rect.x(), flippedY, rect.width(), rect.height());
}

QT_END_NAMESPACE
