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

#define Q_UUIDIMPL
#include <qsqldriverplugin.h>
#include <qstringlist.h>
#ifdef Q_OS_WIN32    // We assume that MS SQL Server is used. Set Q_USE_SYBASE to force Sybase.
// Conflicting declarations of LPCBYTE in sqlfront.h and winscard.h
#define _WINSCARD_H_
#include <windows.h>
#endif
#include "qsql_tds_p.h"

QT_BEGIN_NAMESPACE


// ### Qt6: remove, obsolete since 4.7
class QTDSDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "tds.json")

public:
    QTDSDriverPlugin();

    QSqlDriver* create(const QString &);
};

QTDSDriverPlugin::QTDSDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QTDSDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("QTDS") || name == QLatin1String("QTDS7")) {
        QTDSDriver* driver = new QTDSDriver();
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
