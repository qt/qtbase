// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_sqlite_p.h"
#include "qsql_sqlite_vfs_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QSQLiteDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "sqlite.json")

public:
    QSQLiteDriverPlugin();

    QSqlDriver* create(const QString &) override;
};

QSQLiteDriverPlugin::QSQLiteDriverPlugin()
    : QSqlDriverPlugin()
{
    register_qt_vfs();
}

QSqlDriver* QSQLiteDriverPlugin::create(const QString &name)
{
    if (name == "QSQLITE"_L1) {
        QSQLiteDriver* driver = new QSQLiteDriver();
        return driver;
    }

    return nullptr;
}

QT_END_NAMESPACE

#include "smain.moc"
