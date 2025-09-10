// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qplatformaccessibility.h"
#include <private/qfactoryloader_p.h>
#include "qaccessibleplugin.h"
#include "qaccessibleobject.h"
#include "qaccessiblebridge.h"
#include <QtGui/QGuiApplication>

#include <QDebug>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(accessibility)

/* accessiblebridge plugin discovery stuff */
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, bridgeloader,
    (QAccessibleBridgeFactoryInterface_iid, "/accessiblebridge"_L1))

Q_GLOBAL_STATIC(QList<QAccessibleBridge *>, bridges)

/*!
    \class QPlatformAccessibility
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \ingroup accessibility

    \brief The QPlatformAccessibility class is the base class for
    integrating accessibility backends

    \sa QAccessible
*/
QPlatformAccessibility::QPlatformAccessibility()
    : m_active(false)
{
}

QPlatformAccessibility::~QPlatformAccessibility()
{
}

void QPlatformAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    initialize();

    if (!bridges() || bridges()->isEmpty())
        return;

    for (int i = 0; i < bridges()->size(); ++i)
        bridges()->at(i)->notifyAccessibilityUpdate(event);
}

void QPlatformAccessibility::setRootObject(QObject *o)
{
    initialize();
    if (bridges()->isEmpty())
        return;

    if (!o)
        return;

    for (int i = 0; i < bridges()->size(); ++i) {
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
        bridges()->at(i)->setRootObject(iface);
    }
}

void QPlatformAccessibility::initialize()
{
    static bool isInit = false;
    if (isInit)
        return;
    isInit = true;      // ### not atomic

    typedef QMultiMap<int, QString> PluginKeyMap;
    typedef PluginKeyMap::const_iterator PluginKeyMapConstIterator;

    const PluginKeyMap keyMap = bridgeloader()->keyMap();
    QAccessibleBridgePlugin *factory = nullptr;
    int i = -1;
    const PluginKeyMapConstIterator cend = keyMap.constEnd();
    for (PluginKeyMapConstIterator it = keyMap.constBegin(); it != cend; ++it) {
        if (it.key() != i) {
            i = it.key();
            factory = qobject_cast<QAccessibleBridgePlugin*>(bridgeloader()->instance(i));
        }
        if (factory)
            if (QAccessibleBridge *bridge = factory->create(it.value()))
                bridges()->append(bridge);
    }
}

void QPlatformAccessibility::cleanup()
{
    qDeleteAll(*bridges());
}

void QPlatformAccessibility::setActive(bool active)
{
    m_active = active;
    QAccessible::setActive(active);
}

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE
