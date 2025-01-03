// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqldriverplugin.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSqlDriverPlugin
    \brief The QSqlDriverPlugin class provides an abstract base for custom QSqlDriver plugins.

    \ingroup plugins
    \inmodule QtSql

    The SQL driver plugin is a simple plugin interface that makes it
    easy to create your own SQL driver plugins that can be loaded
    dynamically by Qt.

    Writing a SQL plugin is achieved by subclassing this base class,
    reimplementing the pure virtual function create(), and
    exporting the class with the Q_PLUGIN_METADATA() macro. See the SQL
    plugins that come with Qt for example implementations (in the
    \c{plugins/src/sqldrivers} subdirectory of the source
    distribution).

    The json file containing the metadata for the plugin contains a list of
    keys indicating the supported sql drivers

    \code
    { "Keys": [ "mysqldriver" ] }
    \endcode

    \sa {How to Create Qt Plugins}
*/

/*!
    \fn QSqlDriver *QSqlDriverPlugin::create(const QString& key)

    Creates and returns a QSqlDriver object for the driver called \a
    key. The driver key is usually the class name of the required
    driver. Keys are case sensitive.

    \sa {How to Create Qt Plugins}
*/

/*!
    Constructs a SQL driver plugin and sets the parent to \a parent.
    This is invoked automatically by the moc generated code that exports the plugin.
*/

QSqlDriverPlugin::QSqlDriverPlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the SQL driver plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QSqlDriverPlugin::~QSqlDriverPlugin()
{
}

QT_END_NAMESPACE

#include "moc_qsqldriverplugin.cpp"
