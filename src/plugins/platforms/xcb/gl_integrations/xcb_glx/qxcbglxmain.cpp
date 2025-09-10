// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbglintegrationplugin.h"

#include "qxcbglxintegration.h"

QT_BEGIN_NAMESPACE

class QXcbGlxIntegrationPlugin : public QXcbGlIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QXcbGlIntegrationFactoryInterface_iid FILE "xcb_glx.json")
public:
    QXcbGlIntegration *create() override
    {
        return new QXcbGlxIntegration();
    }

};

QT_END_NAMESPACE

#include "qxcbglxmain.moc"
