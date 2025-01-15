// Copyright (C) 2025 The Qt Company Ltd.
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
#include <memory>

using namespace Qt::Literals;

Q_DECLARE_JNI_CLASS(BenchQtAbstractItemModel,
                    "org/qtproject/qt/android/benchmark/BenchQtAbstractItemModel")

class BenchNativeAbstractItemModel : public QAbstractItemModel {
    Q_OBJECT
    int m_rows = 1;
    int m_cols = 1;

    public:

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_cols;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_rows;
    }

    QVariant data(const QModelIndex &index, int role) const override {
         return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override {
        return createIndex(row, column, quintptr(0));
    }

    QModelIndex parent(const QModelIndex &index) const override
    {
        return QModelIndex();
    }

    QHash<int, QByteArray> roleNames() const override {
        static QHash<int, QByteArray> roles = {
                {0, "integerRole"}
        };
        return roles;
    }

};

class tst_BenchAndroidItemModel : public QObject
{
    Q_OBJECT
    QtJniTypes::BenchQtAbstractItemModel m_jModel;
    std::unique_ptr<QAbstractItemModel> qProxy;
    std::unique_ptr<QAbstractItemModel> nativeModel;

private slots:
    void init();

    void proxiedData();
    void nativeData();

    void proxiedRowCount();
    void nativeRowCount();

    void proxiedColumnCount();
    void nativeColumnCount();

    void proxiedIndex();
    void nativeIndex();
};

void tst_BenchAndroidItemModel::init()
{
    m_jModel = QJniObject::construct<QtJniTypes::BenchQtAbstractItemModel>();
    QVERIFY(m_jModel.isValid());
    qProxy = std::unique_ptr<QAbstractItemModel>(QAndroidItemModelProxy::createNativeProxy(jModel));
    nativeModel = std::make_unique<BenchNativeAbstractItemModel>();
    QVERIFY(qProxy);
}

void tst_BenchAndroidItemModel::proxiedData()
{
    QCOMPARE(qProxy->rowCount(), 1);
    QCOMPARE(qProxy->columnCount(), 1);

    QModelIndex idx = qProxy->index(0, 0, QModelIndex());

    QBENCHMARK { qProxy->data(idx, 0); }
}

void tst_BenchAndroidItemModel::nativeData()
{
    QCOMPARE(nativeModel->rowCount(), 1);
    QCOMPARE(nativeModel->columnCount(), 1);

    QModelIndex idx = nativeModel->index(0, 0, QModelIndex());

    QBENCHMARK { nativeModel->data(idx, 0); }
}

void tst_BenchAndroidItemModel::proxiedRowCount()
{
    QBENCHMARK { qProxy->rowCount(QModelIndex()); }
}

void tst_BenchAndroidItemModel::nativeRowCount()
{
    QBENCHMARK { nativeModel->rowCount(QModelIndex()); }
}

void tst_BenchAndroidItemModel::proxiedColumnCount()
{
    QBENCHMARK { qProxy->columnCount(QModelIndex()); }
}

void tst_BenchAndroidItemModel::nativeColumnCount()
{
    QBENCHMARK { nativeModel->columnCount(QModelIndex()); }
}

void tst_BenchAndroidItemModel::proxiedIndex()
{
    QBENCHMARK { qProxy->index(0, 0, QModelIndex()); }
}

void tst_BenchAndroidItemModel::nativeIndex()
{
    QBENCHMARK { nativeModel->index(0, 0, QModelIndex()); }
}

#include "tst_bench_androiditemmodel.moc"

QTEST_MAIN(tst_BenchAndroidItemModel)
