// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgenericplugin.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGenericPlugin
    \ingroup plugins
    \inmodule QtGui

    \brief The QGenericPlugin class is an abstract base class for
    plugins.

    A mouse plugin can be created by subclassing
    QGenericPlugin and reimplementing the pure virtual create()
    function. By exporting the derived class using the
    Q_PLUGIN_METADATA() macro, The default implementation of the
    QGenericPluginFactory class will automatically detect the plugin and
    load the driver into the server application at run-time. See \l
    {How to Create Qt Plugins} for details.

    The json metadata file should contain a list of keys supported by this
    plugin.

    \sa QGenericPluginFactory
*/

/*!
    Constructs a plugin with the given \a parent.

    Note that this constructor is invoked automatically by the
    moc generated code that exports the plugin, so there is no need for calling it
    explicitly.
*/
QGenericPlugin::QGenericPlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the plugin.

    Note that Qt destroys a plugin automatically when it is no longer
    used, so there is no need for calling the destructor explicitly.
*/
QGenericPlugin::~QGenericPlugin()
{
}

/*!
    \fn QObject* QGenericPlugin::create(const QString &key, const QString& specification)

    Implement this function to create a driver matching the type
    specified by the given \a key and \a specification parameters. Note that
    keys are case-insensitive.
*/

QT_END_NAMESPACE

#include "moc_qgenericplugin.cpp"
