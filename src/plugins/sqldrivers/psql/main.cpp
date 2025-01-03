// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_psql_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QPSQLDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "psql.json")

public:
    QPSQLDriverPlugin();

    QSqlDriver* create(const QString &) override;
};

QPSQLDriverPlugin::QPSQLDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QPSQLDriverPlugin::create(const QString &name)
{
    if (name == "QPSQL"_L1)
        return new QPSQLDriver;
    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
