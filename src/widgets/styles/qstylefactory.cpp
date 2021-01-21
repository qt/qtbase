/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qstylefactory.h"
#include "qstyleplugin.h"
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qapplication.h"
#include "qwindowsstyle_p.h"
#if QT_CONFIG(style_fusion)
#include "qfusionstyle_p.h"
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
    Depending on the platform, "windowsvista"
    and "macintosh" may be available.
    Note that keys are case insensitive.

    \sa QStyle
*/

/*!
    Creates and returns a QStyle object that matches the given \a key, or
    returns \nullptr if no matching style is found.

    Both built-in styles and styles from style plugins are queried for a
    matching style.

    \note The keys used are case insensitive.

    \sa keys()
*/
QStyle *QStyleFactory::create(const QString& key)
{
    QStyle *ret = nullptr;
    QString style = key.toLower();
#if QT_CONFIG(style_windows)
    if (style == QLatin1String("windows"))
        ret = new QWindowsStyle;
    else
#endif
#if QT_CONFIG(style_fusion)
    if (style == QLatin1String("fusion"))
        ret = new QFusionStyle;
    else
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
#if QT_CONFIG(style_windows)
    if (!list.contains(QLatin1String("Windows")))
        list << QLatin1String("Windows");
#endif
#if QT_CONFIG(style_fusion)
    if (!list.contains(QLatin1String("Fusion")))
        list << QLatin1String("Fusion");
#endif
    return list;
}

QT_END_NAMESPACE
