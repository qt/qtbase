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

#include "qiosorientationlistener.h"
#include "qiosscreen.h"
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

Qt::ScreenOrientation convertToQtOrientation(UIDeviceOrientation uiDeviceOrientation)
{
    Qt::ScreenOrientation qtOrientation;
    switch (uiDeviceOrientation) {
        case UIDeviceOrientationPortraitUpsideDown:
            qtOrientation = Qt::InvertedPortraitOrientation;
            break;
        case UIDeviceOrientationLandscapeLeft:
           qtOrientation = Qt::InvertedLandscapeOrientation;
            break;
        case UIDeviceOrientationLandscapeRight:
            qtOrientation = Qt::LandscapeOrientation;
            break;
        case UIDeviceOrientationFaceUp:
        case UIDeviceOrientationFaceDown:
            qtOrientation = static_cast<Qt::ScreenOrientation>(-1); // not supported ATM.
            break;
        default:
            qtOrientation = Qt::PortraitOrientation;
            break;
    }
    return qtOrientation;
}

QT_END_NAMESPACE

@implementation QIOSOrientationListener

- (id) initWithQIOSScreen:(QIOSScreen *)screen
{
    self = [super init];
    if (self) {
        m_screen = screen;
        m_orientation = convertToQtOrientation([UIDevice currentDevice].orientation);
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(orientationChanged:)
            name:@"UIDeviceOrientationDidChangeNotification" object:nil];
    }
    return self;
}

- (void) dealloc
{
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    [super dealloc];
}

- (void) orientationChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    Qt::ScreenOrientation qtOrientation = convertToQtOrientation([UIDevice currentDevice].orientation);
    if (qtOrientation != -1) {
        m_orientation = qtOrientation;
        QWindowSystemInterface::handleScreenOrientationChange(m_screen->screen(), m_orientation);
    }
}

@end

