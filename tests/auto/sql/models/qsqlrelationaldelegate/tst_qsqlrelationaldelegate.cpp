// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QtSql/QtSql>
#include <QTableView>
#include <QComboBox>

#include "../../kernel/qsqldatabase/tst_databases.h"

class tst_QSqlRelationalDelegate : public QObject
{
    Q_OBJECT

public:
    void recreateTestTables(QSqlDatabase);

    tst_Databases dbs;
    tst_QSqlRelationalDelegate();

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

tst_QSqlRelationalDelegate::tst_QSqlRelationalDelegate()
{
}

void tst_QSqlRelationalDelegate::initTestCase_data()
{
    QVERIFY(dbs.open());
    if (dbs.fillTestTable() == 0)
        QSKIP("No database drivers are available in this Qt configuration");
}

void tst_QSqlRelationalDelegate::recreateTestTables(QSqlDatabase db)
{
    const auto reltest1 = qTableName("reltest1", __FILE__, db);
    const auto reltest2 = qTableName("reltest2", __FILE__, db);

    dropTestTables(db);

    QSqlQuery q(db);
    const auto idField = db.driver()->escapeIdentifier(QLatin1String("id"), QSqlDriver::FieldName);
    const auto nameField = db.driver()->escapeIdentifier(QLatin1String("name"), QSqlDriver::FieldName);
    const auto titleKeyField = db.driver()->escapeIdentifier(QLatin1String("title_key"),
                                                             QSqlDriver::FieldName);
    const auto anotherTitleField = db.driver()->escapeIdentifier(QLatin1String("another_title_key"),
                                                                 QSqlDriver::FieldName);
    const auto titleField = db.driver()->escapeIdentifier(QLatin1String("title"),
                                                          QSqlDriver::FieldName);
    QVERIFY_SQL(q, exec("create table " + reltest1 +
            " (" + idField + " int not null primary key, " + nameField + " varchar(20), " +
            titleKeyField + " int, " + anotherTitleField + "int)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(1, 'harry', 1, 2)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(2, 'trond', 2, 1)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(3, 'vohi', 1, 2)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(4, 'boris', 2, 2)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(5, 'nat', NULL, NULL)"));
    QVERIFY_SQL(q, exec("insert into " + reltest1 + " values(6, 'ale', NULL, 2)"));

    QVERIFY_SQL(q, exec("create table " + reltest2 + " (" + idField + " int not null primary key, "
                        + titleKeyField + " varchar(20))"));
    QVERIFY_SQL(q, exec("insert into " + reltest2 + " values(1, 'herr')"));
    QVERIFY_SQL(q, exec("insert into " + reltest2 + " values(2, 'mister')"));
}

void tst_QSqlRelationalDelegate::initTestCase()
{
    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QSqlQuery q(db);
        QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Interbase) {
            q.exec("SET DIALECT 3");
        } else if (dbType == QSqlDriver::MSSqlServer) {
            q.exec("SET ANSI_DEFAULTS ON");
            q.exec("SET IMPLICIT_TRANSACTIONS OFF");
        } else if (dbType == QSqlDriver::PostgreSQL) {
            q.exec("set client_min_messages='warning'");
        }
        recreateTestTables(db);
    }
}

void tst_QSqlRelationalDelegate::cleanupTestCase()
{
    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        dropTestTables(QSqlDatabase::database(dbName));
    }
    dbs.close();
}

void tst_QSqlRelationalDelegate::dropTestTables(QSqlDatabase db)
{
    QStringList tableNames = { qTableName("reltest1", __FILE__, db), qTableName("reltest2", __FILE__, db) };
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

    const auto reltest1 = qTableName("reltest1", __FILE__, db);
    const auto reltest2 = qTableName("reltest2", __FILE__, db);
    const auto idField = db.driver()->escapeIdentifier(QLatin1String("id"), QSqlDriver::FieldName);
    const auto nameField = db.driver()->escapeIdentifier(QLatin1String("name"), QSqlDriver::FieldName);
    const auto titleKeyField = db.driver()->escapeIdentifier(QLatin1String("title_key"),
                                                             QSqlDriver::FieldName);
    QTableView tv;
    QSqlRelationalTableModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, idField, titleKeyField));
    model.setRelation(3, QSqlRelation(reltest2, idField, titleKeyField));
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
    QCOMPARE(comboBoxes.size(), 1);

    QComboBox *editor = comboBoxes.at(0);
    QCOMPARE(editor->currentText(), "herr");
    QTest::keyClick(editor, Qt::Key_Down);
    QTest::keyClick(editor, Qt::Key_Enter);
    QCOMPARE(editor->currentText(), "mister");
    QTest::keyClick(tv.viewport(), Qt::Key_Tab);
    QVERIFY_SQL(model, submitAll());

    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec("SELECT " + titleKeyField + " FROM " + reltest1 + " WHERE " + idField + "=1"));
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toString(), "2");
}

QTEST_MAIN(tst_QSqlRelationalDelegate)
#include "tst_qsqlrelationaldelegate.moc"
