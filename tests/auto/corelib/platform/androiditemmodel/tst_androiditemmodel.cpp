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
#include <QSignalSpy>
#include <QAbstractItemModelTester>

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

    enum DataRole{
        ROLE_STRING = Qt::UserRole,
        ROLE_BOOLEAN,
        ROLE_INTEGER,
        ROLE_DOUBLE,
        ROLE_LONG
    };

private slots:
    void initTestCase_data();
    void init();
    void cleanup();
    void nonDestructiveChecks();
    void addRow();
    void addColumn();
    void removeRow();
    void removeColumn();
    void roleNames();
    void fetchMore();
    void hasIndex();
    void data();
    void setData_data();
    void setData();
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

void tst_AndroidItemModel::nonDestructiveChecks()
{
    QFETCH_GLOBAL(bool, isList);

    for (int i = 0; i < 10; ++i)
        jModel.callMethod<void>("addRow");

    if (!isList) {
        for (int i = 0; i < 10; ++i)
            jModel.callMethod<void>("addCol");
    }

    QAbstractItemModelTester tester(qProxy);
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
    const static QHash<int, QByteArray> expectedRoles = { { ROLE_STRING, "stringRole" },
                                                          { ROLE_BOOLEAN, "booleanRole" },
                                                          { ROLE_INTEGER, "integerRole" },
                                                          { ROLE_DOUBLE, "doubleRole" },
                                                          { ROLE_LONG, "longRole" } };
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
    const static QHash<int, QMetaType::Type> roleToType = { { ROLE_STRING, QMetaType::QString },
                                                            { ROLE_BOOLEAN, QMetaType::Bool },
                                                            { ROLE_INTEGER, QMetaType::Int },
                                                            { ROLE_DOUBLE, QMetaType::Double },
                                                            { ROLE_LONG, QMetaType::Long } };
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
                case ROLE_STRING:
                    QCOMPARE(data.toString(),
                             "r%1/c%2"_L1.arg(QString::number(r), QString::number(c)));
                    break;
                case ROLE_BOOLEAN:
                    QCOMPARE(data.toBool(), ((r + c) % 2) == 0);
                    break;
                case ROLE_INTEGER:
                    QCOMPARE(data.toInt(), (c << 8) + r);
                    break;
                case ROLE_DOUBLE:
                    QVERIFY(qFuzzyCompare(data.toDouble(), (1.0 + r) / (1.0 + c)));
                    break;
                case ROLE_LONG:
                    QCOMPARE(data.toULongLong(), ((c << 8) * (r << 8)));
                    break;
                }
            }
        }
    }
}

void tst_AndroidItemModel::setData_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("role");

    QTest::newRow("role0") << 0 << 0 << int(ROLE_STRING);
    QTest::newRow("role1") << 0 << 0 << int(ROLE_BOOLEAN);
    QTest::newRow("role2") << 0 << 0 << int(ROLE_INTEGER);
}

void tst_AndroidItemModel::setData()
{
    QFETCH_GLOBAL(bool, isList);
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(int, role);

    QSignalSpy spy(qProxy, &QAbstractItemModel::dataChanged);

    jModel.callMethod<void>("addRow");
    if (!isList)
        jModel.callMethod<void>("addCol");

    QCOMPARE_EQ(qProxy->rowCount(), 1);
    QCOMPARE_EQ(qProxy->columnCount(), 1);

    JQtModelIndex index = jModel.callMethod<JQtModelIndex>("index", row, column, JQtModelIndex());
    QVERIFY(jModel.callMethod<jboolean>("setData", index, QJniObject(Void()), role));
    QTRY_COMPARE(spy.count(), 1);

    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);

    const auto topLeft = qvariant_cast<QModelIndex>(arguments.at(0));
    const auto bottomRight = qvariant_cast<QModelIndex>(arguments.at(1));
    const auto roles = qvariant_cast<QList<int>>(arguments.at(2));

    QCOMPARE(topLeft, qProxy->index(row, column, QModelIndex()));
    QCOMPARE(bottomRight, qProxy->index(row, column, QModelIndex()));
    QCOMPARE(roles, QList<int>{role});

    QTRY_COMPARE(jModel.getField<jint>("m_dataChangedCount"), 1);
}

void tst_AndroidItemModel::resetModel()
{
    jModel.callMethod<void>("reset");
    QCOMPARE_EQ(qProxy->rowCount(), 0);
}

#include "tst_androiditemmodel.moc"

QTEST_MAIN(tst_AndroidItemModel)
