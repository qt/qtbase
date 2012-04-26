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


#include <qpa/qplatformintegrationplugin.h>
#include <QtCore/QStringList>

#include "qwindowsintegration.h"

QT_BEGIN_NAMESPACE

/*!
    \group qt-lighthouse-win
    \title Qt Lighthouse plugin for Windows

    \brief Class documentation of the  Qt Lighthouse plugin for Windows.

    \section1 Supported parameters

    The following parameters can be passed on to the -platform argument
    of QGuiApplication:

    \list
    \li \c fontengine=native Indicates that native font engine should be used (default)
    \li \c fontengine=freetype Indicates that freetype font engine should be used
    \li \c gl=gdi Indicates that ARB Open GL functionality should not be used
    \endlist

    \section1 Tips

    \list
    \li The environment variable \c QT_LIGHTHOUSE_WINDOWS_VERBOSE controls
       the debug level. It takes the form
       \c{<keyword1>:<level1>,<keyword2>:<level2>}, where
       keyword is one of \c integration, \c windows, \c backingstore and
       \c fonts. Level is an integer 0..9.
    \endlist
 */

/*!
    \class QWindowsIntegrationPlugin
    \brief Plugin.
    \ingroup qt-lighthouse-win
 */

/*!
    \namespace QtWindows

    \brief Namespace for enumerations, etc.
    \ingroup qt-lighthouse-win
*/

/*!
    \enum QtWindows::WindowsEventType

    \brief Enumerations for WM_XX events.

    With flags that should help to structure the code.

    \ingroup qt-lighthouse-win
*/

class QWindowsIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QPlatformIntegrationFactoryInterface" FILE "windows.json")
public:
    QStringList keys() const;
    QPlatformIntegration *create(const QString&, const QStringList&);
};

QStringList QWindowsIntegrationPlugin::keys() const
{
    return QStringList(QStringLiteral("windows"));
}

QPlatformIntegration *QWindowsIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    if (system.compare(system, QStringLiteral("windows"), Qt::CaseInsensitive) == 0)
        return new QWindowsIntegration;
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
