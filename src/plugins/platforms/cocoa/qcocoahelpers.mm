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

#include "qcocoahelpers.h"

#include <QtCore>
#include <QtGui>

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

