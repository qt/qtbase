/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>

#ifndef QT_NO_ACCESSIBILITY

#include "qaccessibleplugin.h"
#include "qaccessible.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAccessiblePlugin
    \brief The QAccessiblePlugin class provides an abstract base class
    for plugins provinding accessibility information for user interface elements.

    \ingroup plugins
    \ingroup accessibility

    Writing an accessibility plugin is achieved by subclassing this
    base class, reimplementing the pure virtual function create(),
    and exporting the class with the Q_PLUGIN_METADATA() macro.

    \sa {How to Create Qt Plugins}
*/

/*!
    Constructs an accessibility plugin with the given \a parent. This
    is invoked automatically by the plugin loader.
*/
QAccessiblePlugin::QAccessiblePlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the accessibility plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QAccessiblePlugin::~QAccessiblePlugin()
{
}

/*!
    \fn QAccessibleInterface *QAccessiblePlugin::create(const QString &key, QObject *object)

    Creates and returns a QAccessibleInterface implementation for the
    class \a key and the object \a object. Keys are case sensitive.
*/

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
