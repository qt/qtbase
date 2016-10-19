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

#include "qopenwfdintegration.h"
#include "qopenwfdscreen.h"
#include "qopenwfdnativeinterface.h"
#include "qopenwfddevice.h"
#include "qopenwfdwindow.h"
#include "qopenwfdglcontext.h"
#include "qopenwfdbackingstore.h"

#include <QtPlatformSupport/private/qgenericunixprintersupport_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>

#include <stdio.h>

#include <WF/wfd.h>

QOpenWFDIntegration::QOpenWFDIntegration()
    : QPlatformIntegration()
    , mPrinterSupport(new QGenericUnixPrinterSupport)
{
    int numberOfDevices = wfdEnumerateDevices(0,0,0);

    WFDint devices[numberOfDevices];
    int actualNumberOfDevices = wfdEnumerateDevices(devices,numberOfDevices,0);
    Q_ASSERT(actualNumberOfDevices == numberOfDevices);

    mDevices.reserve(actualNumberOfDevices);
    for (int i = 0; i < actualNumberOfDevices; i++) {
        mDevices.append(new QOpenWFDDevice(this,devices[i]));
    }

    mFontDatabase = new QGenericUnixFontDatabase();
    mNativeInterface = new QOpenWFDNativeInterface;
}

QOpenWFDIntegration::~QOpenWFDIntegration()
{
    //don't delete screens since they are deleted by the devices
    qDebug("deleting platform integration");
    for (int i = 0; i < mDevices.size(); i++) {
        delete mDevices[i];
    }

    delete mFontDatabase;
    delete mNativeInterface;
    delete mPrinterSupport;
}

bool QOpenWFDIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QOpenWFDIntegration::createPlatformWindow(QWindow *window) const
{
    return new QOpenWFDWindow(window);
}

QPlatformOpenGLContext *QOpenWFDIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QOpenWFDScreen *screen = static_cast<QOpenWFDScreen *>(context->screen()->handle());

    return new QOpenWFDGLContext(screen->port()->device());
}

QPlatformBackingStore *QOpenWFDIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QOpenWFDBackingStore(window);
}

QAbstractEventDispatcher *QOpenWFDIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformFontDatabase *QOpenWFDIntegration::fontDatabase() const
{
    return mFontDatabase;
}

QPlatformNativeInterface * QOpenWFDIntegration::nativeInterface() const
{
    return mNativeInterface;
}

QPlatformPrinterSupport * QOpenWFDIntegration::printerSupport() const
{
    return mPrinterSupport;
}

void QOpenWFDIntegration::addScreen(QOpenWFDScreen *screen)
{
    screenAdded(screen);
}

void QOpenWFDIntegration::destroyScreen(QOpenWFDScreen *screen)
{
    QPlatformIntegration::destroyScreen(screen);
}
