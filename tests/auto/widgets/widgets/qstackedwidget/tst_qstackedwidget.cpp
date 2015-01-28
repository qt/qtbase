/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qstackedwidget.h>
#include <qpushbutton.h>
#include <QHBoxLayout>
#include <qlineedit.h>

class tst_QStackedWidget : public QObject
{
Q_OBJECT

public:
    tst_QStackedWidget();
    virtual ~tst_QStackedWidget();

private slots:
    void getSetCheck();
    void testMinimumSize();
    void dynamicPages();
};

tst_QStackedWidget::tst_QStackedWidget()
{
}

tst_QStackedWidget::~tst_QStackedWidget()
{
}

// Testing that stackedwidget respect the minimum size of it's contents (task 95319)
void tst_QStackedWidget::testMinimumSize()
{
    QWidget w;
    QStackedWidget sw(&w);
    QPushButton button("Text", &sw);
    sw.addWidget(&button);
    QHBoxLayout hboxLayout;
    hboxLayout.addWidget(&sw);
    w.setLayout(&hboxLayout);
    w.show();
    QVERIFY(w.minimumSize() != QSize(0, 0));
}

// Testing get/set functions
void tst_QStackedWidget::getSetCheck()
{
    QStackedWidget obj1;
    // int QStackedWidget::currentIndex()
    // void QStackedWidget::setCurrentIndex(int)
    obj1.setCurrentIndex(0);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(-1, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(-1, obj1.currentIndex());

    // QWidget * QStackedWidget::currentWidget()
    // void QStackedWidget::setCurrentWidget(QWidget *)
    QWidget *var2 = new QWidget();
    obj1.addWidget(var2);
    obj1.setCurrentWidget(var2);
    QCOMPARE(var2, obj1.currentWidget());

// Disabled, task to fix is 128939.
#if 0
    // Layouts assert on any unknown widgets here, 0-pointers included.
    // This seems wrong behavior, since the setCurrentIndex(int), which
    // is really a convenience function for setCurrentWidget(QWidget*),
    // has no problem handling out-of-bounds indices.
    // ("convenience function" => "just another way of achieving the
    // same goal")
    obj1.setCurrentWidget((QWidget *)0);
    QCOMPARE(obj1.currentWidget(), var2);
#endif
    delete var2;
}

// QTBUG-18242, a widget that deletes its children in hideEvent().
// This caused a crash in QStackedLayout::setCurrentIndex() since
// the focus widget was destroyed while hiding the previous page.
class TestPage : public QWidget
{
public:
    TestPage (bool staticWidgets = false) : QWidget(0), m_staticWidgets(staticWidgets)
    {
        new QVBoxLayout (this);
    }

    ~TestPage() {
        destroyWidgets();
    }

    void setN(int n)
    {
        m_n = n;
        if (m_staticWidgets)
            createWidgets();
    }

    virtual void showEvent (QShowEvent *)
    {
        if (!m_staticWidgets)
            createWidgets();
    }

    virtual void hideEvent (QHideEvent *)
    {
        if (!m_staticWidgets)
            destroyWidgets();
    }

private:
    void createWidgets() {
        for (int i = 0; i < m_n; ++i) {
            QLineEdit *le = new QLineEdit(this);
            le->setObjectName(QString::fromLatin1("lineEdit%1").arg(i));
            layout ()->addWidget(le);
            m_les << le;
        }
    }

    void destroyWidgets()
    {
        qDeleteAll(m_les);
        m_les.clear ();
    }

    int m_n;
    const bool m_staticWidgets;
    QList<QLineEdit*> m_les;
};

void tst_QStackedWidget::dynamicPages()
{
    QStackedWidget *sw = new QStackedWidget;

    TestPage *w1 = new TestPage(true);
    w1->setN(3);

    TestPage *w2 = new TestPage;
    w2->setN(3);

    sw->addWidget(w1);
    sw->addWidget(w2);

    QLineEdit *le11 = w1->findChild<QLineEdit*>(QLatin1String("lineEdit1"));
    le11->setFocus();   // set focus to second widget in the page
    sw->resize(200, 200);
    sw->show();
    qApp->setActiveWindow(sw);
    QTest::qWaitForWindowActive(sw);
    QTRY_COMPARE(QApplication::focusWidget(), le11);

    sw->setCurrentIndex(1);
    QLineEdit *le22 = w2->findChild<QLineEdit*>(QLatin1String("lineEdit2"));
    le22->setFocus();
    QTRY_COMPARE(QApplication::focusWidget(), le22);
    // Going back should move focus back to le11
    sw->setCurrentIndex(0);
    QTRY_COMPARE(QApplication::focusWidget(), le11);

}

QTEST_MAIN(tst_QStackedWidget)
#include "tst_qstackedwidget.moc"
