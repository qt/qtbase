// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QTest>

QT_BEGIN_NAMESPACE
namespace QtSharedPointer {
    Q_CORE_EXPORT void internalSafetyCheckCleanCheck();
}
QT_END_NAMESPACE

class tst_QSharedPointer_and_QWidget: public QObject
{
    Q_OBJECT
private slots:
    void weak_externalDelete();
    void weak_parentDelete();
    void weak_parentDelete_setParent();

    void strong_weak();

    void strong_sharedptrDelete();

public slots:
    void cleanup() { safetyCheck(); }

public:
    inline void safetyCheck()
    {
#ifdef QT_BUILD_INTERNAL
        QtSharedPointer::internalSafetyCheckCleanCheck();
#endif
    }
};

void tst_QSharedPointer_and_QWidget::weak_externalDelete()
{
    QWidget *w = new QWidget;
    QPointer<QWidget> ptr = w;

    QVERIFY(!ptr.isNull());

    delete w;
    QVERIFY(ptr.isNull());
}

void tst_QSharedPointer_and_QWidget::weak_parentDelete()
{
    QWidget *parent = new QWidget;
    QWidget *w = new QWidget(parent);
    QPointer<QWidget> ptr = w;

    QVERIFY(!ptr.isNull());

    delete parent;
    QVERIFY(ptr.isNull());
}

void tst_QSharedPointer_and_QWidget::weak_parentDelete_setParent()
{
    QWidget *parent = new QWidget;
    QWidget *w = new QWidget;
    QPointer<QWidget> ptr = w;
    w->setParent(parent);

    QVERIFY(!ptr.isNull());

    delete parent;
    QVERIFY(ptr.isNull());
}

// -- mixed --

void tst_QSharedPointer_and_QWidget::strong_weak()
{
    QSharedPointer<QWidget> ptr(new QWidget);
    QPointer<QWidget> weak = ptr.data();
    QWeakPointer<QWidget> weak2 = ptr;

    QVERIFY(!weak.isNull());
    QVERIFY(!weak2.isNull());

    ptr.clear(); // deletes

    QVERIFY(weak.isNull());
    QVERIFY(weak2.isNull());
}


// ---- strong management ----

void tst_QSharedPointer_and_QWidget::strong_sharedptrDelete()
{
    QWidget *parent = new QWidget;
    QSharedPointer<QWidget> ptr(new QWidget(parent));
    QWeakPointer<QWidget> weak = ptr;
    QPointer<QWidget> check = ptr.data();

    QVERIFY(!check.isNull());
    QVERIFY(!weak.isNull());

    ptr.clear();  // deletes

    QVERIFY(check.isNull());
    QVERIFY(weak.isNull());

    delete parent; // mustn't crash
}

QTEST_MAIN(tst_QSharedPointer_and_QWidget)

#include "tst_qsharedpointer_and_qwidget.moc"
