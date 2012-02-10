/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qguiplatformplugin_p.h"
#include <qdebug.h>
#include <qfile.h>
#include <qdir.h>
#include <qsettings.h>
#include "private/qfactoryloader_p.h"
#include "qstylefactory.h"
#include "qapplication.h"
#include "qplatformdefs.h"
#include "qicon.h"

#ifdef Q_OS_WINCE
extern bool qt_wince_is_smartphone(); //qguifunctions_wince.cpp
extern bool qt_wince_is_mobile();     //qguifunctions_wince.cpp
extern bool qt_wince_is_pocket_pc();  //qguifunctions_wince.cpp
#endif


#if defined(Q_WS_X11)
#include <private/qkde_p.h>
#include <private/qgtkstyle_p.h>
#include <private/qt_x11_p.h>
#endif


QT_BEGIN_NAMESPACE


/*! \internal
    Return (an construct if necesseray) the Gui Platform plugin.

    The plugin key to be loaded is inside the QT_PLATFORM_PLUGIN environment variable.
    If it is not set, it will be the DESKTOP_SESSION on X11.

    If no plugin can be loaded, the default one is returned.
 */
QGuiPlatformPlugin *qt_guiPlatformPlugin()
{
    static QGuiPlatformPlugin *plugin;
    if (!plugin)
    {
#ifndef QT_NO_LIBRARY

        QString key = QString::fromLocal8Bit(qgetenv("QT_PLATFORM_PLUGIN"));
#ifdef Q_WS_X11
        if (key.isEmpty()) {
            switch(X11->desktopEnvironment) {
            case DE_KDE:
                key = QString::fromLatin1("kde");
                break;
            default:
                key = QString::fromLocal8Bit(qgetenv("DESKTOP_SESSION"));
                break;
            }
        }
#endif

        if (!key.isEmpty() && QApplication::desktopSettingsAware()) {
            QFactoryLoader loader(QGuiPlatformPluginInterface_iid, QLatin1String("/gui_platform"));
            plugin = qobject_cast<QGuiPlatformPlugin *>(loader.instance(key));
        }
#endif // QT_NO_LIBRARY

        if(!plugin) {
            static QGuiPlatformPlugin def;
            plugin = &def;
        }
    }
    return plugin;
}


/* \class QPlatformPlugin
    QGuiPlatformPlugin can be used to integrate Qt applications in a platform built on top of Qt.
    The application developer should not know or use the plugin, it is only used by Qt internaly.

    But full platforms that are built on top of Qt may provide a plugin so 3rd party Qt applications
    running on the platform are integrated.
 */

/*
    The constructor can be used to install hooks in Qt
 */
QGuiPlatformPlugin::QGuiPlatformPlugin(QObject *parent) : QObject(parent) {}
QGuiPlatformPlugin::~QGuiPlatformPlugin() {}

/* backend for QFileIconProvider,  null icon means default */
QIcon QGuiPlatformPlugin::fileSystemIcon(const QFileInfo &)
{
    return QIcon();
}

QT_END_NAMESPACE
