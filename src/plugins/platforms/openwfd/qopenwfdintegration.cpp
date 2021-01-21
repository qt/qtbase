/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
#include <qpa/qwindowsysteminterface.h>

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
