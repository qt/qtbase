// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblebridge.h"

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

/*!
    \class QAccessibleBridge
    \brief The QAccessibleBridge class is the base class for
    accessibility back-ends.
    \internal

    \ingroup accessibility
    \inmodule QtWidgets

    Qt supports Microsoft Active Accessibility (MSAA), \macos
    Accessibility, and the Unix/X11 AT-SPI standard. By subclassing
    QAccessibleBridge, you can support other backends than the
    predefined ones.

    Currently, custom bridges are only supported on Unix. We might
    add support for them on other platforms as well if there is
    enough demand.

    \sa QAccessible, QAccessibleBridgePlugin
*/

/*!
    \fn QAccessibleBridge::~QAccessibleBridge()

    Destroys the accessibility bridge object.
*/
QAccessibleBridge::~QAccessibleBridge()
    = default;

/*!
    \fn void QAccessibleBridge::setRootObject(QAccessibleInterface *object)

    This function is called by Qt at application startup to set the
    root accessible object of the application to \a object. All other
    accessible objects in the application can be reached by the
    client using object navigation.
*/

/*!
    \fn void QAccessibleBridge::notifyAccessibilityUpdate(QAccessibleEvent *event)

    This function is called by Qt to notify the bridge about a change
    in the accessibility information. The \a event specifies the interface,
    object, reason and child element that has changed.

    \sa QAccessible::updateAccessibility()
*/

/*!
    \class QAccessibleBridgePlugin
    \brief The QAccessibleBridgePlugin class provides an abstract
    base for accessibility bridge plugins.
    \internal

    \ingroup plugins
    \ingroup accessibility
    \inmodule QtWidgets

    Writing an accessibility bridge plugin is achieved by subclassing
    this base class, reimplementing the pure virtual function create(),
    and exporting the class with the Q_PLUGIN_METADATA() macro.

    \sa QAccessibleBridge, QAccessiblePlugin, {How to Create Qt Plugins}
*/

/*!
    Constructs an accessibility bridge plugin with the given \a
    parent. This is invoked automatically by the plugin loader.
*/
QAccessibleBridgePlugin::QAccessibleBridgePlugin(QObject *parent)
    : QObject(parent)
{

}

/*!
    Destroys the accessibility bridge plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QAccessibleBridgePlugin::~QAccessibleBridgePlugin()
{

}

/*!
    \fn QAccessibleBridge *QAccessibleBridgePlugin::create(const QString &key)

    Creates and returns the QAccessibleBridge object corresponding to
    the given \a key. Keys are case sensitive.

    \sa keys()
*/

QT_END_NAMESPACE

#include "moc_qaccessiblebridge.cpp"

#endif // QT_CONFIG(accessibility)
