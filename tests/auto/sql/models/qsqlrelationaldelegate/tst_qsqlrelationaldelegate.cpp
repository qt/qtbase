/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtSql/QtSql>
#include <QTableView>
#include <QComboBox>

#include "../../kernel/qsqldatabase/tst_databases.h"

const QString reltest1(qTableName("reltest1", __FILE__, QSqlDatabase())),
              reltest2(qTableName("reltest2", __FILE__, QSqlDatabase()));

class tst_QSqlRelationalDelegate : public QObject
{
    Q_OBJECT

public:
    void recreateTestTables(QSqlDatabase);

    tst_Databases dbs;

public slots:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void comboBoxEditor();
private:
    void dropTestTables(QSqlDatabase db);
};


void tst_QSqlRelationalDelegate::initTestCase_data()
{
    QVERIFY(dbs.open());
    if (dbs.fillTestTable() == 0)
        QSKIP("No database drivers are available in this Qt configuration");
}

void tst_QSqlRelationalDelegate::recreateTestTables(QSqlDatabase db)
{
    dropTestTables(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("create table " + reltest1 +
            " (id int not null primary key, name varchar(20), title_key int, another_title_key int)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(1, 'harry', 1, 2)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(2, 'trond', 2, 1)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(3, 'vohi', 1, 2)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(4, 'boris', 2, 2)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(5, 'nat', NULL, NULL)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(6, 'ale', NULL, 2)"));

    QVERIFY_SQL(q, exec("create table " + reltest2 + " (id int not null primary key, title varchar(20))"));
    QVERIFY_SQL(q, exec("insert into " + reltest2 + " values(1, 'herr')"));
    QVERIFY_SQL(q, exec("insert into " + reltest2 + " values(2, 'mister')"));
}

void tst_QSqlRelationalDelegate::initTestCase()
{
    foreach (const QString &dbname, dbs.dbNames) {
        QSqlDatabase db=QSqlDatabase::database(dbname);
        QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Interbase) {
            db.exec("SET DIALECT 3");
        } else if (dbType == QSqlDriver::MSSqlServer) {
            db.exec("SET ANSI_DEFAULTS ON");
            db.exec("SET IMPLICIT_TRANSACTIONS OFF");
        } else if (dbType == QSqlDriver::PostgreSQL) {
            db.exec("set client_min_messages='warning'");
        }
        recreateTestTables(db);
    }
}

void tst_QSqlRelationalDelegate::cleanupTestCase()
{
    foreach (const QString &dbName, dbs.dbNames) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        dropTestTables(QSqlDatabase::database(dbName));
    }
    dbs.close();
}

void tst_QSqlRelationalDelegate::dropTestTables(QSqlDatabase db)
{
    QStringList tableNames = { reltest1, reltest2 };
    tst_Databases::safeDropTables(db, tableNames);
}

void tst_QSqlRelationalDelegate::init()
{
}

void tst_QSqlRelationalDelegate::cleanup()
{
}

void tst_QSqlRelationalDelegate::comboBoxEditor()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QTableView tv;
    QSqlRelationalTableModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    model.setRelation(3, QSqlRelation(reltest2, "id", "title"));
    tv.setModel(&model);
    QVERIFY_SQL(model, select());

    QSqlRelationalDelegate delegate;
    tv.setItemDelegate(&delegate);
    tv.show();
    QVERIFY(QTest::qWaitForWindowActive(&tv));

    QModelIndex index = model.index(0, 2);
    tv.setCurrentIndex(index);
    tv.edit(index);
    QList<QComboBox*> comboBoxes = tv.viewport()->findChildren<QComboBox *>();
    QCOMPARE(comboBoxes.count(), 1);

    QComboBox *editor = comboBoxes.at(0);
    QCOMPARE(editor->currentText(), "herr");
    QTest::keyClick(editor, Qt::Key_Down);
    QTest::keyClick(editor, Qt::Key_Enter);
    QCOMPARE(editor->currentText(), "mister");
    QTest::keyClick(tv.viewport(), Qt::Key_Tab);
    QVERIFY_SQL(model, submitAll());

    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec("SELECT title_key FROM " + reltest1 + " WHERE id=1"));
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toString(), "2");
}

QTEST_MAIN(tst_QSqlRelationalDelegate)
#include "tst_qsqlrelationaldelegate.moc"
