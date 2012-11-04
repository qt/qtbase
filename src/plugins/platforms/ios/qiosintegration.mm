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

#include "qiosIntegration.h"
#include "qioswindow.h"
#include "qiosbackingstore.h"
#include "qiosscreen.h"
#include "qioseventdispatcher.h"

#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QIOSIntegration::QIOSIntegration()
    :m_fontDb(new QCoreTextFontDatabase), m_screen(new QIOSScreen(QIOSScreen::MainScreen))
{
    screenAdded(m_screen);
}

QIOSIntegration::~QIOSIntegration()
{
}

QPlatformPixmap *QIOSIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    Q_UNUSED(type);
    qDebug() << __FUNCTION__ << "not yet implemented";
    return 0;
    //return new QRasterPixmapData(type);
}

QPlatformWindow *QIOSIntegration::createPlatformWindow(QWindow *window) const
{
    return new QIOSWindow(window);
}

QPlatformBackingStore *QIOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QIOSBackingStore(window);
}

QAbstractEventDispatcher *QIOSIntegration::guiThreadEventDispatcher() const
{
    return new QIOSEventDispatcher();
}

QPlatformFontDatabase * QIOSIntegration::fontDatabase() const
{
    return m_fontDb;
}

QT_END_NAMESPACE
