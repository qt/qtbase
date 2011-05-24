/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qscreendriverfactory_qws.h"

#include "qscreen_qws.h"
#include "qapplication.h"
#include "qscreenlinuxfb_qws.h"
#include "qscreentransformed_qws.h"
#include "qscreenvfb_qws.h"
#include "qscreenmulti_qws_p.h"
#include "qscreenqnx_qws.h"
#include "qscreenintegrityfb_qws.h"
#include <stdlib.h>
#include "private/qfactoryloader_p.h"
#include "qscreendriverplugin_qws.h"
#ifndef QT_NO_QWS_DIRECTFB
#include "qdirectfbscreen.h"
#endif
#ifndef QT_NO_QWS_VNC
#include "qscreenvnc_qws.h"
#endif

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QScreenDriverFactoryInterface_iid,
     QLatin1String("/gfxdrivers"), Qt::CaseInsensitive))

#endif //QT_NO_LIBRARY
#endif //QT_MAKEDLL

/*!
    \class QScreenDriverFactory
    \ingroup qws

    \brief The QScreenDriverFactory class creates screen drivers in
    Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    QScreenDriverFactory is used to detect and instantiate the
    available screen drivers, allowing \l{Qt for Embedded Linux} to load the
    preferred driver into the server application at runtime.  The
    create() function returns a QScreen object representing the screen
    driver identified by a given key. The valid keys (i.e. the
    supported drivers) can be retrieved using the keys() function.


    \l{Qt for Embedded Linux} provides several built-in screen drivers. In
    addition, custom screen drivers can be added using Qt's plugin
    mechanism, i.e. by subclassing the QScreen class and creating a
    screen driver plugin (QScreenDriverPlugin). See the
    \l{Qt for Embedded Linux Display Management}{display management}
    documentation for details.

    \sa QScreen, QScreenDriverPlugin
*/

/*!
    Creates the screen driver specified by the given \a key, using the
    display specified by the given \a displayId.

    Note that the keys are case-insensitive.

    \sa keys()
*/
QScreen *QScreenDriverFactory::create(const QString& key, int displayId)
{
    QString driver = key.toLower();
#if defined(Q_OS_QNX) && !defined(QT_NO_QWS_QNX)
    if (driver == QLatin1String("qnx") || driver.isEmpty())
        return new QQnxScreen(displayId);
#endif
#if defined(Q_OS_INTEGRITY) && !defined(QT_NO_QWS_INTEGRITY)
    if (driver == QLatin1String("integrityfb") || driver.isEmpty())
        return new QIntfbScreen(displayId);
#endif
#ifndef QT_NO_QWS_QVFB
    if (driver == QLatin1String("qvfb") || driver.isEmpty())
        return new QVFbScreen(displayId);
#endif
#ifndef QT_NO_QWS_LINUXFB
    if (driver == QLatin1String("linuxfb") || driver.isEmpty())
        return new QLinuxFbScreen(displayId);
#endif
#ifndef QT_NO_QWS_DIRECTFB
    if (driver == QLatin1String("directfb") || driver.isEmpty())
        return new QDirectFBScreen(displayId);
#endif
#ifndef QT_NO_QWS_TRANSFORMED
    if (driver == QLatin1String("transformed"))
        return new QTransformedScreen(displayId);
#endif
#ifndef QT_NO_QWS_VNC
    if (driver == QLatin1String("vnc"))
        return new QVNCScreen(displayId);
#endif
#ifndef QT_NO_QWS_MULTISCREEN
    if (driver == QLatin1String("multi"))
        return new QMultiScreen(displayId);
#endif
#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY

    if (QScreenDriverFactoryInterface *factory = qobject_cast<QScreenDriverFactoryInterface*>(loader()->instance(key)))
        return factory->create(driver, displayId);

#endif
#endif
    return 0;
}

/*!
    Returns the list of valid keys, i.e. the available screen drivers.

    \sa create()
*/
QStringList QScreenDriverFactory::keys()
{
    QStringList list;

#if defined(Q_OS_QNX) && !defined(QT_NO_QWS_QNX)
    list << QLatin1String("QNX");
#endif
#if defined(Q_OS_INTEGRITY) && !defined(QT_NO_QWS_INTEGRITY)
    list << QLatin1String("INTEGRITYFB");
#endif
#ifndef QT_NO_QWS_QVFB
    list << QLatin1String("QVFb");
#endif
#ifndef QT_NO_QWS_LINUXFB
    list << QLatin1String("LinuxFb");
#endif
#ifndef QT_NO_QWS_TRANSFORMED
    list << QLatin1String("Transformed");
#endif
#ifndef QT_NO_QWS_VNC
    list << QLatin1String("VNC");
#endif
#ifndef QT_NO_QWS_MULTISCREEN
    list << QLatin1String("Multi");
#endif

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY
    QStringList plugins = loader()->keys();
    for (int i = 0; i < plugins.size(); ++i) {
# ifdef QT_NO_QWS_QVFB
        // give QVFb top priority for autodetection
        if (plugins.at(i) == QLatin1String("QVFb"))
            list.prepend(plugins.at(i));
        else
# endif
        if (!list.contains(plugins.at(i)))
            list += plugins.at(i);
    }
#endif //QT_NO_LIBRARY
#endif //QT_MAKEDLL
    return list;
}

QT_END_NAMESPACE
