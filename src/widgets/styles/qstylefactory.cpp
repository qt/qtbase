// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QStyleFactoryInterface_iid, "/styles"_L1, Qt::CaseInsensitive))

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
    and "macos" may be available.
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
    if (style == "windows"_L1)
        ret = new QWindowsStyle;
    else
#endif
#if QT_CONFIG(style_fusion)
    if (style == "fusion"_L1)
        ret = new QFusionStyle;
    else
#endif
#if defined(Q_OS_MACOS) && QT_DEPRECATED_SINCE(6, 0)
    if (style == "macintosh"_L1) {
        qWarning() << "The style key 'macintosh' is deprecated. Please use 'macos' instead.";
        style = QStringLiteral("macos");
    } else
#endif
    { } // Keep these here - they make the #ifdefery above work
    if (!ret)
        ret = qLoadPlugin<QStyle, QStylePlugin>(loader(), style);
    if (ret) {
        ret->setObjectName(style);
        ret->setName(style);
    }
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
    if (!list.contains("Windows"_L1))
        list << "Windows"_L1;
#endif
#if QT_CONFIG(style_fusion)
    if (!list.contains("Fusion"_L1))
        list << "Fusion"_L1;
#endif
    return list;
}

QT_END_NAMESPACE
