/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_sqlite2_p.h"

QT_BEGIN_NAMESPACE

// ### Qt6: remove, obsolete since 5.14
class QSQLite2DriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "sqlite2.json")

public:
    QSQLite2DriverPlugin();

    QSqlDriver* create(const QString &);
};

QSQLite2DriverPlugin::QSQLite2DriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QSQLite2DriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("QSQLITE2")) {
        QSQLite2Driver* driver = new QSQLite2Driver();
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE

#include "smain.moc"
