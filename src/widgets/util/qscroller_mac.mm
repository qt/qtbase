/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#ifdef Q_DEAD_CODE_FROM_QT4_MAC

#import <Cocoa/Cocoa.h>

#include "qscroller_p.h"

QT_BEGIN_NAMESPACE

QPointF QScrollerPrivate::realDpi(int screen)
{
    QMacAutoReleasePool pool;
    NSArray *nsscreens = [NSScreen screens];

    if (screen < 0 || screen >= int([nsscreens count]))
        screen = 0;

    NSScreen *nsscreen = [nsscreens objectAtIndex:screen];
    CGDirectDisplayID display = [[[nsscreen deviceDescription] objectForKey:@"NSScreenNumber"] intValue];

    CGSize mmsize = CGDisplayScreenSize(display);
    if (mmsize.width > 0 && mmsize.height > 0) {
        return QPointF(CGDisplayPixelsWide(display) / mmsize.width,
                       CGDisplayPixelsHigh(display) / mmsize.height) * qreal(25.4);
    } else {
        return QPointF();
    }
}

QT_END_NAMESPACE

#endif
