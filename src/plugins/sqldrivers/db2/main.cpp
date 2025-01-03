// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_db2_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QDB2DriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "db2.json")

public:
    QDB2DriverPlugin();

    QSqlDriver* create(const QString &);
};

QDB2DriverPlugin::QDB2DriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QDB2DriverPlugin::create(const QString &name)
{
    if (name == "QDB2"_L1) {
        QDB2Driver* driver = new QDB2Driver();
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
