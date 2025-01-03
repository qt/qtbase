// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_odbc_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QODBCDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "odbc.json")

public:
    QODBCDriverPlugin();

    QSqlDriver* create(const QString &) override;
};

QODBCDriverPlugin::QODBCDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QODBCDriverPlugin::create(const QString &name)
{
    if (name == "QODBC"_L1) {
        QODBCDriver* driver = new QODBCDriver();
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
