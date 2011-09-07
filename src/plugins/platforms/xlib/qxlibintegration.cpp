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

#include <private/qguiapplication_p.h>
#include "qxlibintegration.h"
#include "qxlibbackingstore.h"
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>

#include "qxlibwindow.h"
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include "qxlibscreen.h"
#include "qxlibclipboard.h"
#include "qxlibdisplay.h"
#include "qxlibnativeinterface.h"
#include "qglxintegration.h"

QT_BEGIN_NAMESPACE

QXlibIntegration::QXlibIntegration()
    : mFontDb(new QGenericUnixFontDatabase())
    , mClipboard(0)
    , mNativeInterface(new QXlibNativeInterface)
{
    mEventDispatcher = createUnixEventDispatcher();
    QGuiApplicationPrivate::instance()->setEventDispatcher(mEventDispatcher);

    XInitThreads();

    mPrimaryScreen = new QXlibScreen();
    mScreens.append(mPrimaryScreen);
    screenAdded(mPrimaryScreen);
}

bool QXlibIntegration::hasCapability(QPlatformIntegration::Capability) const
{
    return true;
}

QPlatformBackingStore *QXlibIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QXlibBackingStore(window);
}

QPlatformOpenGLContext *QXlibIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXlibScreen *screen = static_cast<QXlibScreen *>(context->screen()->handle());

    return new QGLXContext(screen, context->format(), context->shareHandle());
}


QPlatformWindow *QXlibIntegration::createPlatformWindow(QWindow *window) const
{
    return new QXlibWindow(window);
}

QAbstractEventDispatcher *QXlibIntegration::guiThreadEventDispatcher() const
{
    return mEventDispatcher;
}

QPlatformFontDatabase *QXlibIntegration::fontDatabase() const
{
    return mFontDb;
}

QPlatformClipboard * QXlibIntegration::clipboard() const
{
    //Use lazy init since clipboard needs QTestliteScreen
    if (!mClipboard) {
        QXlibIntegration *that = const_cast<QXlibIntegration *>(this);
        that->mClipboard = new QXlibClipboard(mPrimaryScreen);
    }
    return mClipboard;
}

QPlatformNativeInterface * QXlibIntegration::nativeInterface() const
{
    return mNativeInterface;
}

QT_END_NAMESPACE
