/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosscreen.h"
#include "qioswindow.h"

#include <QtGui/QGuiApplication>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QIOSScreen::QIOSScreen(unsigned int screenIndex)
    : QPlatformScreen()
    , m_uiScreen([[UIScreen screens] objectAtIndex:qMin(screenIndex, [[UIScreen screens] count] - 1)])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    CGRect bounds = [m_uiScreen bounds];
    CGFloat scale = [m_uiScreen scale];
    updateInterfaceOrientation();

    m_format = QImage::Format_ARGB32_Premultiplied;

    m_depth = 24;

    const qreal inch = 25.4;
    qreal unscaledDpi = 160.;
    int dragDistance = 12 * scale;
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        unscaledDpi = 132.;
        dragDistance = 10 * scale;
    }
    m_physicalSize = QSize(qRound(bounds.size.width * inch / unscaledDpi), qRound(bounds.size.height * inch / unscaledDpi));

    //qApp->setStartDragDistance(dragDistance);

    /*
    QFont font; // system font is helvetica, so that is fine already
    font.setPixelSize([UIFont systemFontSize] * scale);
    qApp->setFont(font);
     */

    [pool release];
}

QIOSScreen::~QIOSScreen()
{
}

UIScreen *QIOSScreen::uiScreen() const
{
    return m_uiScreen;
}

void QIOSScreen::updateInterfaceOrientation()
{
    qDebug() << __FUNCTION__ << "not implemented";
    /*
    CGRect bounds = [uiScreen() bounds];
    CGFloat scale = [uiScreen() scale];
    switch ([[UIApplication sharedApplication] statusBarOrientation]) {
    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:
        m_geometry = QRect(bounds.origin.x * scale, bounds.origin.y * scale,
                           bounds.size.width * scale, bounds.size.height * scale);;
        break;
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
        m_geometry = QRect(bounds.origin.x * scale, bounds.origin.y * scale,
                           bounds.size.height * scale, bounds.size.width * scale);
        break;
    }
    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        QIOSWindow *platformWindow = static_cast<QIOSWindow *>(widget->platformWindow());
        if (platformWindow && platformWindow->platformScreen() == this) {
            platformWindow->updateGeometryAndOrientation();
        }
    }
    */
}

QT_END_NAMESPACE
