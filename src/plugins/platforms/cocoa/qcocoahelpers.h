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

#ifndef QCOCOAHELPERS_H
#define QCOCOAHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It provides helper functions
// for the Cocoa lighthouse plugin. This header file may
// change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qcore_mac_p.h>
#include <Cocoa/Cocoa.h>

class QString;

// Conversion functions
QStringList qt_mac_NSArrayToQStringList(void *nsarray);
void *qt_mac_QStringListToNSMutableArrayVoid(const QStringList &list);

inline NSMutableArray *qt_mac_QStringListToNSMutableArray(const QStringList &qstrlist)
{ return reinterpret_cast<NSMutableArray *>(qt_mac_QStringListToNSMutableArrayVoid(qstrlist)); }

inline QString qt_mac_NSStringToQString(const NSString *nsstr)
{ return QCFString::toQString(reinterpret_cast<const CFStringRef>(nsstr)); }

inline NSString *qt_mac_QStringToNSString(const QString &qstr)
{ return [reinterpret_cast<const NSString *>(QCFString::toCFStringRef(qstr)) autorelease]; }

// Misc

void qt_mac_transformProccessToForegroundApplication();



#endif //QCOCOAHELPERS_H

