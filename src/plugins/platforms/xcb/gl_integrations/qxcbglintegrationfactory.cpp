// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbglintegrationfactory.h"
#include "qxcbglintegrationplugin.h"

#include "qxcbglintegrationplugin.h"
#include "private/qfactoryloader_p.h"
#include "qguiapplication.h"
#include "qdir.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QXcbGlIntegrationFactoryInterface_iid, "/xcbglintegrations"_L1, Qt::CaseInsensitive))

QXcbGlIntegration *QXcbGlIntegrationFactory::create(const QString &platform)
{
    return qLoadPlugin<QXcbGlIntegration, QXcbGlIntegrationPlugin>(loader(), platform);
}

QT_END_NAMESPACE
