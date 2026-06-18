// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include <qsqldriverplugin.h>
#include "qsql_firebird_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QFirebirdDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "firebird.json")

public:
    QFirebirdDriverPlugin() : QSqlDriverPlugin() {}

    QSqlDriver *create(const QString &name) override
    {
        if (name == "QFIREBIRD"_L1)
            return new QFirebirdDriver();
        return nullptr;
    }
};

QT_END_NAMESPACE

#include "main.moc"
