// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgenericpluginfactory.h"

#include "qguiapplication.h"
#include "private/qfactoryloader_p.h"
#include "qgenericplugin.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, gpLoader,
    (QGenericPluginFactoryInterface_iid, "/generic"_L1, Qt::CaseInsensitive))

/*!
    \class QGenericPluginFactory
    \ingroup plugins
    \inmodule QtGui

    \brief The QGenericPluginFactory class creates plugin drivers.

    \sa QGenericPlugin
*/

/*!
    Creates the driver specified by \a key, using the given \a specification.

    Note that the keys are case-insensitive.

    \sa keys()
*/
QObject *QGenericPluginFactory::create(const QString& key, const QString &specification)
{
    return qLoadPlugin<QObject, QGenericPlugin>(gpLoader(), key.toLower(), specification);
}

/*!
    Returns the list of valid keys, i.e. the available mouse drivers.

    \sa create()
*/
QStringList QGenericPluginFactory::keys()
{
    QStringList list;

    typedef QMultiMap<int, QString> PluginKeyMap;
    typedef PluginKeyMap::const_iterator PluginKeyMapConstIterator;

    const PluginKeyMap keyMap = gpLoader()->keyMap();
    const PluginKeyMapConstIterator cend = keyMap.constEnd();
    for (PluginKeyMapConstIterator it = keyMap.constBegin(); it != cend; ++it)
        if (!list.contains(it.value()))
            list += it.value();
    return list;
}

QT_END_NAMESPACE
