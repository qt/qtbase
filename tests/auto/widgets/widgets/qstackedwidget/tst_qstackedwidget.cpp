// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qstackedwidget.h>
#include <qpushbutton.h>
#include <QHBoxLayout>
#include <qlineedit.h>

#include <QtWidgets/private/qapplication_p.h>

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

    virtual void showEvent (QShowEvent *) override
    {
        if (!m_staticWidgets)
            createWidgets();
    }

    virtual void hideEvent (QHideEvent *) override
    {
        if (!m_staticWidgets)
            destroyWidgets();
    }

private:
    void createWidgets() {
        for (int i = 0; i < m_n; ++i) {
            QLineEdit *le = new QLineEdit(this);
            le->setObjectName(QLatin1String("lineEdit") + QString::number(i));
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
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStackedWidget stackedWidget;
    QStackedWidget *sw = &stackedWidget;

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
    QApplicationPrivate::setActiveWindow(sw);
    QVERIFY(QTest::qWaitForWindowActive(sw));
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
