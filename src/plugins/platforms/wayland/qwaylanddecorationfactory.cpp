// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylanddecorationfactory_p.h"
#include "qwaylanddecorationplugin_p.h"

#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qwdfiLoader,
    (QWaylandDecorationFactoryInterface_iid, QLatin1String("/wayland-decoration-client"), Qt::CaseInsensitive))

QStringList QWaylandDecorationFactory::keys()
{
    return qwdfiLoader->keyMap().values();
}

QWaylandAbstractDecoration *QWaylandDecorationFactory::create(const QString &name, const QStringList &args)
{
    return qLoadPlugin<QWaylandAbstractDecoration, QWaylandDecorationPlugin>(qwdfiLoader(), name, args);
}

}

QT_END_NAMESPACE
