/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
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

#ifndef QSQLDRIVERPLUGIN_H
#define QSQLDRIVERPLUGIN_H

#include <QtSql/qtsqlglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE


class QSqlDriver;

#define QSqlDriverFactoryInterface_iid "org.qt-project.Qt.QSqlDriverFactoryInterface"

class Q_SQL_EXPORT QSqlDriverPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QSqlDriverPlugin(QObject *parent = nullptr);
    ~QSqlDriverPlugin();

    virtual QSqlDriver *create(const QString &key) = 0;

};

QT_END_NAMESPACE

#endif // QSQLDRIVERPLUGIN_H
