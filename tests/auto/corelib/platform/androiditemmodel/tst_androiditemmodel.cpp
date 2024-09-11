// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QtCore/private/qandroiditemmodelproxy_p.h>
#include <QtCore/private/qandroidmodelindexproxy_p.h>
#include <QtCore/private/qandroidtypes_p.h>

#include <QGuiApplication>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qstring.h>

using namespace Qt::Literals;

Q_DECLARE_JNI_CLASS(TestQtAbstractItemModel,
                    "org/qtproject/qt/android/tests/TestQtAbstractItemModel")
Q_DECLARE_JNI_CLASS(TestQtAbstractListModel,
                    "org/qtproject/qt/android/tests/TestQtAbstractListModel")

class tst_AndroidItemModel : public QObject
{
    Q_OBJECT
    QJniObject jModel;
    QAbstractItemModel *qProxy;
    void resetModel();

private slots:
    void initTestCase_data();
    void init();
    void cleanup();
    void addRow();
    void addColumn();
    void removeRow();
    void removeColumn();
    void roleNames();
    void fetchMore();
    void hasIndex();
    void data();
};

void tst_AndroidItemModel::initTestCase_data()
{
    QTest::addColumn<QJniObject>("JavaModel");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<bool>("isList");
    QTest::newRow("TestItemModel")
            << QJniObject::construct<QtJniTypes::TestQtAbstractItemModel>() << 3 << false;
    QTest::newRow("TestListModel")
            << QJniObject::construct<QtJniTypes::TestQtAbstractListModel>() << 1 << true;
}

void tst_AndroidItemModel::init()
{
    QFETCH_GLOBAL(QJniObject, JavaModel);
    jModel = JavaModel;
    QVERIFY(jModel.isValid());
    qProxy = QAndroidItemModelProxy::createNativeProxy(jModel);
    QVERIFY(qProxy);
}

void tst_AndroidItemModel::cleanup()
{
    resetModel();
}

void tst_AndroidItemModel::addRow()
{
    const int rowsBefore = qProxy->rowCount();
    jModel.callMethod<void>("addRow");
    QCOMPARE_EQ(qProxy->rowCount(), rowsBefore + 1);
}

void tst_AndroidItemModel::addColumn()
{
    QFETCH_GLOBAL(bool, isList);
    if (isList)
        QSKIP("This test function requires a two-dimensional model.");

    const int columnsBefore = qProxy->columnCount();
    jModel.callMethod<void>("addCol");
    QCOMPARE_EQ(qProxy->columnCount(), columnsBefore + 1);
}

void tst_AndroidItemModel::removeRow()
{
    jModel.callMethod<void>("addRow");
    jModel.callMethod<void>("addRow");
    QCOMPARE_EQ(qProxy->rowCount(), 2);
    jModel.callMethod<void>("removeRow");
    QCOMPARE_EQ(qProxy->rowCount(), 1);
    jModel.callMethod<void>("removeRow");
    QCOMPARE_EQ(qProxy->rowCount(), 0);
}

void tst_AndroidItemModel::removeColumn()
{
    QFETCH_GLOBAL(bool, isList);
    if (isList)
        QSKIP("This test function requires a two-dimensional model.");

    jModel.callMethod<void>("addCol");
    jModel.callMethod<void>("addCol");
    QCOMPARE_EQ(qProxy->columnCount(), 2);
    jModel.callMethod<void>("removeCol");
    QCOMPARE_EQ(qProxy->columnCount(), 1);
    jModel.callMethod<void>("removeCol");
    QCOMPARE_EQ(qProxy->columnCount(), 0);
}

void tst_AndroidItemModel::roleNames()
{
    const static QHash<int, QByteArray> expectedRoles = { { 0, "stringRole" },
                                                          { 1, "booleanRole" },
                                                          { 2, "integerRole" },
                                                          { 3, "doubleRole" },
                                                          { 4, "longRole" } };
    QCOMPARE(qProxy->roleNames(), expectedRoles);
}

void tst_AndroidItemModel::fetchMore()
{
    // In the Java TestModel :
    // canFetchMore() returns true when row count is less than 30
    // fetchMore() adds 10 rows at most, or the remaining until row count is 30
    QVERIFY(qProxy->canFetchMore(QModelIndex()));
    qProxy->fetchMore(QModelIndex());
    QCOMPARE_EQ(qProxy->rowCount(), 10);
    QVERIFY(qProxy->canFetchMore(QModelIndex()));
    qProxy->fetchMore(QModelIndex());
    QCOMPARE_EQ(qProxy->rowCount(), 20);
    jModel.callMethod<void>("addRow");
    QVERIFY(qProxy->canFetchMore(QModelIndex()));
    qProxy->fetchMore(QModelIndex());
    QCOMPARE_EQ(qProxy->rowCount(), 30);
    QVERIFY(!qProxy->canFetchMore(QModelIndex()));
}

void tst_AndroidItemModel::hasIndex()
{
    QFETCH_GLOBAL(int, columnCount);
    QFETCH_GLOBAL(bool, isList);

    if (!isList) {
        for (int i = 0; i < columnCount; ++i)
            jModel.callMethod<void>("addCol");
    }

    qProxy->fetchMore(QModelIndex());
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < columnCount; ++c) {
            QVERIFY(qProxy->hasIndex(r, c));
        }
    }
}

void tst_AndroidItemModel::data()
{
    const static QHash<int, QMetaType::Type> roleToType = { { 0, QMetaType::QString },
                                                            { 1, QMetaType::Bool },
                                                            { 2, QMetaType::Int },
                                                            { 3, QMetaType::Double },
                                                            { 4, QMetaType::Long } };
    QFETCH_GLOBAL(int, columnCount);
    QFETCH_GLOBAL(bool, isList);

    if (!isList) {
        for (int i = 0; i < columnCount; ++i)
            jModel.callMethod<void>("addCol");
    }

    QVERIFY(qProxy->canFetchMore(QModelIndex()));
    qProxy->fetchMore(QModelIndex());
    QCOMPARE_EQ(qProxy->rowCount(), 10);

    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < columnCount; ++c) {
            QModelIndex index = qProxy->index(r, c);
            for (int role : roleToType.keys()) {
                const QVariant data = qProxy->data(index, role);
                QCOMPARE_EQ(data.typeId(), roleToType[role]);
                switch (role) {
                case 0:
                    QCOMPARE(data.toString(),
                             "r%1/c%2"_L1.arg(QString::number(r), QString::number(c)));
                    break;
                case 1:
                    QCOMPARE(data.toBool(), ((r + c) % 2) == 0);
                    break;
                case 2:
                    QCOMPARE(data.toInt(), (c << 8) + r);
                    break;
                case 3:
                    QVERIFY(qFuzzyCompare(data.toDouble(), (1.0 + r) / (1.0 + c)));
                    break;
                case 4:
                    QCOMPARE(data.toULongLong(), ((c << 8) * (r << 8)));
                    break;
                }
            }
        }
    }
}

void tst_AndroidItemModel::resetModel()
{
    jModel.callMethod<void>("reset");
    QCOMPARE_EQ(qProxy->rowCount(), 0);
}

#include "tst_androiditemmodel.moc"

QTEST_MAIN(tst_AndroidItemModel)
