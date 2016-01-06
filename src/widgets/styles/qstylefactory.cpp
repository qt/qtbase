/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qstylefactory.h"
#include "qstyleplugin.h"
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qapplication.h"
#include "qwindowsstyle_p.h"
#ifndef QT_NO_STYLE_FUSION
#include "qfusionstyle_p.h"
#ifndef QT_NO_STYLE_ANDROID
#include "qandroidstyle_p.h"
#endif
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
#include "qwindowsxpstyle_p.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
#include "qwindowsvistastyle_p.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
#include "qwindowscestyle_p.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
#include "qwindowsmobilestyle_p.h"
#endif

#if !defined(QT_NO_STYLE_MAC) && defined(Q_OS_MAC)
#  include "qmacstyle_mac_p.h"
#endif

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QStyleFactoryInterface_iid, QLatin1String("/styles"), Qt::CaseInsensitive))

/*!
    \class QStyleFactory
    \brief The QStyleFactory class creates QStyle objects.

    \ingroup appearance
    \inmodule QtWidgets

    The QStyle class is an abstract base class that encapsulates the
    look and feel of a GUI. QStyleFactory creates a QStyle object
    using the create() function and a key identifying the style. The
    styles are either built-in or dynamically loaded from a style
    plugin (see QStylePlugin).

    The valid keys can be retrieved using the keys()
    function. Typically they include "windows" and "fusion".
    Depending on the platform, "windowsxp", "windowsvista"
    and "macintosh" may be available.
    Note that keys are case insensitive.

    \sa QStyle
*/

/*!
    Creates and returns a QStyle object that matches the given \a key, or
    returns 0 if no matching style is found.

    Both built-in styles and styles from style plugins are queried for a
    matching style.

    \note The keys used are case insensitive.

    \sa keys()
*/
QStyle *QStyleFactory::create(const QString& key)
{
    QStyle *ret = 0;
    QString style = key.toLower();
#ifndef QT_NO_STYLE_WINDOWS
    if (style == QLatin1String("windows"))
        ret = new QWindowsStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
    if (style == QLatin1String("windowsce"))
        ret = new QWindowsCEStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
    if (style == QLatin1String("windowsmobile"))
        ret = new QWindowsMobileStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
    if (style == QLatin1String("windowsxp"))
        ret = new QWindowsXPStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
    if (style == QLatin1String("windowsvista"))
        ret = new QWindowsVistaStyle;
    else
#endif
#ifndef QT_NO_STYLE_FUSION
    if (style == QLatin1String("fusion"))
        ret = new QFusionStyle;
    else
#endif
#ifndef QT_NO_STYLE_ANDROID
    if (style == QLatin1String("android"))
        ret = new QAndroidStyle;
    else
#endif
#ifndef QT_NO_STYLE_MAC
    if (style.startsWith(QLatin1String("macintosh"))) {
        ret = new QMacStyle;
#  ifdef Q_DEAD_CODE_FROM_QT4_MAC
        if (style == QLatin1String("macintosh"))
            style += QLatin1String(" (aqua)");
#  endif
    } else
#endif
    { } // Keep these here - they make the #ifdefery above work
    if (!ret)
        ret = qLoadPlugin<QStyle, QStylePlugin>(loader(), style);
    if(ret)
        ret->setObjectName(style);
    return ret;
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/
QStringList QStyleFactory::keys()
{
    QStringList list;
    typedef QMultiMap<int, QString> PluginKeyMap;

    const PluginKeyMap keyMap = loader()->keyMap();
    const PluginKeyMap::const_iterator cend = keyMap.constEnd();
    for (PluginKeyMap::const_iterator it = keyMap.constBegin(); it != cend; ++it)
        list.append(it.value());
#ifndef QT_NO_STYLE_WINDOWS
    if (!list.contains(QLatin1String("Windows")))
        list << QLatin1String("Windows");
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
    if (!list.contains(QLatin1String("WindowsCE")))
        list << QLatin1String("WindowsCE");
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
    if (!list.contains(QLatin1String("WindowsMobile")))
        list << QLatin1String("WindowsMobile");
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
    if (!list.contains(QLatin1String("WindowsXP")) &&
        (QSysInfo::WindowsVersion >= QSysInfo::WV_XP && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
        list << QLatin1String("WindowsXP");
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
    if (!list.contains(QLatin1String("WindowsVista")) &&
        (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
        list << QLatin1String("WindowsVista");
#endif
#ifndef QT_NO_STYLE_ANDROID
    if (!list.contains(QLatin1String("Android")))
        list << QLatin1String("Android");
#endif
#ifndef QT_NO_STYLE_FUSION
    if (!list.contains(QLatin1String("Fusion")))
        list << QLatin1String("Fusion");
#endif
#ifndef QT_NO_STYLE_MAC
    QString mstyle = QLatin1String("Macintosh");
# ifdef Q_DEAD_CODE_FROM_QT4_MAC
    mstyle += QLatin1String(" (aqua)");
# endif
    if (!list.contains(mstyle))
        list << mstyle;
#endif
    return list;
}

QT_END_NAMESPACE
