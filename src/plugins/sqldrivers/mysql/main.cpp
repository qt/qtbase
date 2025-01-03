// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_mysql_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QMYSQLDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "mysql.json")

public:
    QMYSQLDriverPlugin();

    QSqlDriver* create(const QString &) override;
};

QMYSQLDriverPlugin::QMYSQLDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QMYSQLDriverPlugin::create(const QString &name)
{
    if (name == "QMYSQL"_L1 || name == "QMARIADB"_L1) {
        QMYSQLDriver* driver = new QMYSQLDriver();
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
