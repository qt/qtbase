// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiconengineplugin.h"
#include "qiconengine.h"

QT_BEGIN_NAMESPACE

/*!
    \class QIconEnginePlugin
    \brief The QIconEnginePlugin class provides an abstract base for custom QIconEngine plugins.

    \ingroup plugins
    \inmodule QtGui

    The icon engine plugin is a simple plugin interface that makes it easy to
    create custom icon engines that can be loaded dynamically into applications
    through QIcon. QIcon uses the file or resource name's suffix to determine
    what icon engine to use.

    Writing a icon engine plugin is achieved by subclassing this base class,
    reimplementing the pure virtual function create(), and
    exporting the class with the Q_PLUGIN_METADATA() macro.

    The json metadata should contain a list of icon engine keys that this plugin supports.
    The keys correspond to the suffix of the file or resource name used when the plugin was
    created. Keys are case insensitive.

    \code
    { "Keys": [ "myiconengine" ] }
    \endcode

    \sa {How to Create Qt Plugins}
*/

/*!
    \fn QIconEngine* QIconEnginePlugin::create(const QString& filename)

    Creates and returns a QIconEngine object for the icon with the given
    \a filename.
*/

/*!
    Constructs a icon engine plugin with the given \a parent. This is invoked
    automatically by the plugin loader.
*/
QIconEnginePlugin::QIconEnginePlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the icon engine plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QIconEnginePlugin::~QIconEnginePlugin()
{
}


QT_END_NAMESPACE

#include "moc_qiconengineplugin.cpp"
