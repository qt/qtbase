// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "plugin.h"

JsonTestPlugin::JsonTestPlugin(QObject *parent)
  : QObject(parent)
{

}

#include "moc_plugin.cpp"
