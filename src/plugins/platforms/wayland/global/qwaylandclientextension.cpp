// Copyright (C) 2017 Erik Larsson.
// Copyright (C) 2021 David Redondo <qt@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandclientextension.h"
#include "qwaylandclientextension_p.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>

QT_BEGIN_NAMESPACE

using RegistryGlobal = QtWaylandClient::QWaylandDisplay::RegistryGlobal;
using namespace Qt::StringLiterals;

QWaylandClientExtensionPrivate::QWaylandClientExtensionPrivate()
{
    // Keep the possibility to use a custom waylandIntegration as a plugin,
    // but also add the possibility to run it as a QML component.
    waylandIntegration = QtWaylandClient::QWaylandIntegration::instance();
    if (!waylandIntegration)
        waylandIntegration = new QtWaylandClient::QWaylandIntegration("wayland"_L1);
}

void QWaylandClientExtensionPrivate::globalAdded(const RegistryGlobal &global)
{
    Q_Q(QWaylandClientExtension);
    if (!active && global.interface == QLatin1String(q->extensionInterface()->name)) {
        q->bind(global.registry, global.id, global.version);
        active = true;
        emit q->activeChanged();
    }
}

void QWaylandClientExtensionPrivate::globalRemoved(const RegistryGlobal &global)
{
    Q_Q(QWaylandClientExtension);
    if (active && global.interface == QLatin1String(q->extensionInterface()->name)) {
        active = false;
        emit q->activeChanged();
    }
}

/*!
    \class QWaylandClientExtensionTemplate
    \brief A class for implementing custom extensions on the Wayland protocol.
    \inmodule QtWaylandClient

    The QWaylandClientExtensionTemplate is a convenience class for creating
    the client-side implementation of custom Wayland protocols. Typical usage
    involves inheriting this class and instantiating it with its own subclass.

    See the \l{Custom Extension} example in \l{Qt Wayland Compositor} for a
    concrete use of this class.
*/

/*!
    \class QWaylandClientExtension
    \brief A class for implementing custom extensions on the Wayland protocol.
    \inmodule QtWaylandClient

    The QWaylandClientExtension class can be used to implement custom extensions
    for Wayland protocol. The extension must also be supported by the compositor
    in order to be usable. See the \l{Custom Extension} example in
    \l{Qt Wayland Compositor} for an example that implements both the compositor
    and client sides of a custom extension.

    This class is usually not inherited directly, but through
    QWaylandClientExtensionTemplate for convenience.
*/

/*!
   \internal
*/
void QWaylandClientExtension::initialize()
{
    Q_D(QWaylandClientExtension);
    if (d->active) {
        return;
    }
    const QtWaylandClient::QWaylandDisplay *display = d->waylandIntegration->display();
    const auto globals = display->globals();
    auto global =
            std::find_if(globals.cbegin(), globals.cend(), [this](const RegistryGlobal &global) {
                return global.interface == QLatin1String(extensionInterface()->name);
            });
    if (global != globals.cend()) {
        bind(global->registry, global->id, global->version);
        d->active = true;
        emit activeChanged();
    }
}

/*!
   \since 6.12
   Constructs the client extension and sets its version to \a ver and makes the extension a
   child of \a parent.
*/
QWaylandClientExtension::QWaylandClientExtension(int ver, QObject *parent)
    : QObject(*new QWaylandClientExtensionPrivate(), parent)
{
    Q_D(QWaylandClientExtension);
    d->version = ver;
    auto display = d->waylandIntegration->display();
    QObjectPrivate::connect(display, &QtWaylandClient::QWaylandDisplay::globalAdded, d,
                            &QWaylandClientExtensionPrivate::globalAdded);
    QObjectPrivate::connect(display, &QtWaylandClient::QWaylandDisplay::globalRemoved, d,
                            &QWaylandClientExtensionPrivate::globalRemoved);
    // This function uses virtual functions and we don't want it to be called from the constructor.
    QMetaObject::invokeMethod(this, "initialize", Qt::QueuedConnection);
}

/*!
   Constructs the client extension and sets its version to \a ver. Equivalent to
   QWaylandClientExtension(ver, nullptr).
*/
QWaylandClientExtension::QWaylandClientExtension(const int ver)
    : QWaylandClientExtension(ver, nullptr)
{
}

/*!
   Destroys the client extension.
*/
QWaylandClientExtension::~QWaylandClientExtension()
{
}

/*!
   \internal
*/
QtWaylandClient::QWaylandIntegration *QWaylandClientExtension::integration() const
{
    Q_D(const QWaylandClientExtension);
    return d->waylandIntegration;
}

/*!
   \fn const struct wl_interface *extensionInterface() const
   \internal
*/

/*!
   \fn void bind(struct ::wl_registry *registry, int id, int version)
   \internal
*/

/*!
   \property QWaylandClientExtension::protocolVersion
   \brief The version of the protocol.

   This property holds the version of the protocol that has been requested.
*/
int QWaylandClientExtension::version() const
{
    Q_D(const QWaylandClientExtension);
    return d->version;
}

void QWaylandClientExtension::setVersion(const int ver)
{
    Q_D(QWaylandClientExtension);
    if (d->version != ver) {
        d->version = ver;
        emit versionChanged();
    }
}

/*!
   \property QWaylandClientExtension::active
   \brief The active state of the extension.

   Set to \c true if the extension is currently active. Otherwise this
   property is \c false.
*/

bool QWaylandClientExtension::isActive() const
{
    Q_D(const QWaylandClientExtension);
    return d->active;
}

QT_END_NAMESPACE

#include "moc_qwaylandclientextension.cpp"
