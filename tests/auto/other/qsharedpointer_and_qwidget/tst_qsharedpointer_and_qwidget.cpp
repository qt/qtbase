/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtTest/QtTest>

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
