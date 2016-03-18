/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSQL_TDS_H
#define QSQL_TDS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtSql/qsqldriver.h>

#ifdef Q_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef Q_USE_SYBASE
#define DBNTWIN32 // indicates 32bit windows dblib
#endif
#include <winsock2.h>
#include <QtCore/qt_windows.h>
#include <sqlfront.h>
#include <sqldb.h>
#define CS_PUBLIC
#else
#include <sybfront.h>
#include <sybdb.h>
#endif //Q_OS_WIN32

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_TDS
#else
#define Q_EXPORT_SQLDRIVER_TDS Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QSqlResult;
class QTDSDriverPrivate;

class Q_EXPORT_SQLDRIVER_TDS QTDSDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QTDSDriver)
    Q_OBJECT
    friend class QTDSResultPrivate;
public:
    explicit QTDSDriver(QObject* parent = 0);
    QTDSDriver(LOGINREC* rec, const QString& host, const QString &db, QObject* parent = 0);
    ~QTDSDriver();
    bool hasFeature(DriverFeature f) const Q_DECL_OVERRIDE;
    bool open(const QString &db,
               const QString &user,
               const QString &password,
               const QString &host,
               int port,
               const QString &connOpts) Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    QStringList tables(QSql::TableType) const Q_DECL_OVERRIDE;
    QSqlResult *createResult() const Q_DECL_OVERRIDE;
    QSqlRecord record(const QString &tablename) const Q_DECL_OVERRIDE;
    QSqlIndex primaryIndex(const QString &tablename) const Q_DECL_OVERRIDE;

    QString formatValue(const QSqlField &field,
                         bool trimStrings) const Q_DECL_OVERRIDE;
    QVariant handle() const Q_DECL_OVERRIDE;

    QString escapeIdentifier(const QString &identifier, IdentifierType type) const Q_DECL_OVERRIDE;

protected:
    bool beginTransaction() Q_DECL_OVERRIDE;
    bool commitTransaction() Q_DECL_OVERRIDE;
    bool rollbackTransaction() Q_DECL_OVERRIDE;
private:
    void init();
};

QT_END_NAMESPACE

#endif // QSQL_TDS_H
